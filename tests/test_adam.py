import copy
import math
from typing import Optional

import torch
from titans.model.gpt import GPT

from tensornvme import DiskOffloader


def gpt2_toy(**kwargs):
    model_kwargs = dict(hidden_size=8, depth=2, num_heads=2, **kwargs)
    model = GPT(**model_kwargs)
    return model


def adam(step, lr, param, grad, exp_avg, exp_avg_sq, beta1=0.9, beta2=0.999, eps=1e-12):
    exp_avg.mul_(beta1).add_(grad, alpha=1 - beta1)
    exp_avg_sq.mul_(beta2).addcmul_(grad, grad.conj(), value=1 - beta2)

    bias_correction1 = 1 - beta1 ** step
    bias_correction2 = 1 - beta2 ** step
    step_size = lr / bias_correction1
    bias_correction2_sqrt = math.sqrt(bias_correction2)
    denom = (exp_avg_sq.sqrt() / bias_correction2_sqrt).add_(eps)
    param.addcdiv_(exp_avg, denom, value=-step_size)


class Adam(torch.optim.Optimizer):
    def __init__(self, params, lr, betas=(0.9, 0.999), offloader: Optional[DiskOffloader] = None, prefetch: int = 0, vecio: bool = False) -> None:
        default = dict(lr=lr, betas=betas)
        super().__init__(params, default)
        self.offloader = offloader
        self.prefetch = prefetch
        self.vecio = vecio
        self.param_to_group = {}
        # init states
        for group in self.param_groups:
            for p in group['params']:
                if p.requires_grad:
                    self.param_to_group[p] = group
                    state = self.state[p]
                    state['step'] = 0
                    state['exp_avg'] = torch.zeros_like(p)
                    state['exp_avg_sq'] = torch.zeros_like(p)
                    if self.offloader is None:
                        continue
                    if vecio:
                        self.offloader.sync_writev(
                            [state['exp_avg'], state['exp_avg_sq']])
                    else:
                        self.offloader.sync_write(state['exp_avg'])
                        self.offloader.sync_write(state['exp_avg_sq'])

    def step(self, closure=None):
        loss = None
        if closure is not None:
            with torch.enable_grad():
                loss = closure()

        params = [
            p for group in self.param_groups for p in group['params'] if p.grad is not None]
        if self.offloader is not None and self.prefetch > 0:
            for p in params[:self.prefetch]:
                state = self.state[p]
                if self.vecio:
                    self.offloader.sync_readv(
                        [state['exp_avg'], state['exp_avg_sq']])
                else:
                    self.offloader.sync_read(state['exp_avg'])
                    self.offloader.sync_read(state['exp_avg_sq'])

        for i, p in enumerate(params):
            state = self.state[p]
            group = self.param_to_group[p]
            state['step'] += 1
            beta1, beta2 = group['betas']
            self._pre_step(i, params)
            adam(state['step'], group['lr'], p, p.grad, state['exp_avg'],
                 state['exp_avg_sq'], beta1=beta1, beta2=beta2)
            self._post_step(i, params)

        return loss

    def _pre_step(self, idx, params):
        if self.offloader is None:
            return
        if self.prefetch > 0:
            if idx % self.prefetch == 0:
                self.offloader.sync_read_events()
                if idx + self.prefetch < len(params):
                    for prefetch_p in params[idx + self.prefetch:idx + self.prefetch * 2]:
                        prefetch_state = self.state[prefetch_p]
                        if self.vecio:
                            self.offloader.async_readv(
                                [prefetch_state['exp_avg'], prefetch_state['exp_avg_sq']])
                        else:
                            self.offloader.async_read(
                                prefetch_state['exp_avg'])
                            self.offloader.async_read(
                                prefetch_state['exp_avg_sq'])
        else:
            state = self.state[params[idx]]
            if self.vecio:
                self.offloader.sync_readv(
                    [state['exp_avg'], state['exp_avg_sq']])
            else:
                self.offloader.sync_read(state['exp_avg'])
                self.offloader.sync_read(state['exp_avg_sq'])

    def _post_step(self, idx, params):
        if self.offloader is None:
            return
        state = self.state[params[idx]]
        if self.prefetch > 0:
            if idx % self.prefetch == 0:
                self.offloader.sync_write_events()
            if self.vecio:
                self.offloader.async_writev(
                    [state['exp_avg'], state['exp_avg_sq']])
            else:
                self.offloader.async_write(state['exp_avg'])
                self.offloader.async_write(state['exp_avg_sq'])
        else:
            if self.vecio:
                self.offloader.sync_writev(
                    [state['exp_avg'], state['exp_avg_sq']])
            else:
                self.offloader.sync_write(state['exp_avg'])
                self.offloader.sync_write(state['exp_avg_sq'])


def test_adam():
    params = list(gpt2_toy().cpu().parameters())
    for _, p in enumerate(params):
        if p.grad is None and p.requires_grad:
            p.grad = torch.ones_like(p.data, dtype=torch.float) * 0.12345

    params_gt = copy.deepcopy(params)
    for _, p in enumerate(params_gt):
        if p.grad is None and p.requires_grad:
            p.grad = torch.ones_like(p.data, dtype=torch.float) * 0.12345
    optimizer = torch.optim.Adam(params_gt, 1e-3)
    optimizer.step()

    test_config = [
        {'n_entries': 1, 'backend': None, 'prefetch': 0, 'vecio': False},

        {'n_entries': 1, 'backend': 'uring', 'prefetch': 0, 'vecio': False},
        {'n_entries': 8, 'backend': 'uring', 'prefetch': 2, 'vecio': False},

        {'n_entries': 1, 'backend': 'uring', 'prefetch': 0, 'vecio': True},
        {'n_entries': 8, 'backend': 'uring', 'prefetch': 2, 'vecio': True},

        {'n_entries': 1, 'backend': 'aio', 'prefetch': 0, 'vecio': False},
        {'n_entries': 8, 'backend': 'aio', 'prefetch': 2, 'vecio': False},

        {'n_entries': 1, 'backend': 'aio', 'prefetch': 0, 'vecio': True},
        {'n_entries': 8, 'backend': 'aio', 'prefetch': 2, 'vecio': True},
    ]

    for i, cfg in enumerate(test_config):
        params_test = copy.deepcopy(params)
        for _, p in enumerate(params_test):
            if p.grad is None and p.requires_grad:
                p.grad = torch.ones_like(p.data, dtype=torch.float) * 0.12345
        if cfg['backend'] is None:
            offloader = None
        else:
            offloader = DiskOffloader(
                '.', cfg['n_entries'], backend=cfg['backend'])
        optimizer_test = Adam(
            params_test, 1e-3, offloader=offloader, prefetch=cfg['prefetch'], vecio=cfg['vecio'])
        optimizer_test.step()

        for p1, p2, p3 in zip(params_gt, params_test, params):
            assert torch.abs(torch.mean(p1 - p2)) < 1e-8
            assert not torch.equal(p1, p3)


if __name__ == '__main__':
    with torch.no_grad():
        test_adam()
