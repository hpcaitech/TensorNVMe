import torch
import math
from colo_nvme import DiskOffloader
from titans.model.gpt import gpt2_small
from time import time
from typing import List, Optional
from functools import partial

N_WARMUP = 5
N_ACTIVATE = 10


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
                        self.offloader.sync_writev([state['exp_avg'], state['exp_avg_sq']])
                    else:
                        self.offloader.sync_write(state['exp_avg'])
                        self.offloader.sync_write(state['exp_avg_sq'])

    def step(self, closure=None):
        loss = None
        if closure is not None:
            with torch.enable_grad():
                loss = closure()

        params = [p for group in self.param_groups for p in group['params'] if p.grad is not None]
        if self.offloader is not None and self.prefetch > 0:
            for p in params[:self.prefetch]:
                state = self.state[p]
                if self.vecio:
                    self.offloader.sync_readv([state['exp_avg'], state['exp_avg_sq']])
                else:
                    self.offloader.sync_read(state['exp_avg'])
                    self.offloader.sync_read(state['exp_avg_sq'])

        for i, p in enumerate(params):
            state = self.state[p]
            group = self.param_to_group[p]
            state['step'] += 1
            beta1, beta2 = group['betas']
            self._pre_step(i, params)
            adam(state['step'], group['lr'], p, p.grad, state['exp_avg'], state['exp_avg_sq'], beta1=beta1, beta2=beta2)
            self._post_step(i, params)

        return loss

    def _pre_step(self, idx, params):
        if self.offloader is None:
            return
        if self.prefetch > 0:
            if idx % self.prefetch == 0:
                self.offloader.sync_read_events()
            if idx + 1 < len(params):
                for prefetch_p in params[idx + 1:idx + 1 + self.prefetch]:
                    prefetch_state = self.state[prefetch_p]
                    if self.vecio:
                        self.offloader.async_readv([prefetch_state['exp_avg'], prefetch_state['exp_avg_sq']])
                    else:
                        self.offloader.async_read(prefetch_state['exp_avg'])
                        self.offloader.async_read(prefetch_state['exp_avg_sq'])
        else:
            state = self.state[params[idx]]
            if self.vecio:
                self.offloader.sync_readv([state['exp_avg'], state['exp_avg_sq']])
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
                self.offloader.async_writev([state['exp_avg'], state['exp_avg_sq']])
            else:
                self.offloader.async_write(state['exp_avg'])
                self.offloader.async_write(state['exp_avg_sq'])
        else:
            if self.vecio:
                self.offloader.sync_writev([state['exp_avg'], state['exp_avg_sq']])
            else:
                self.offloader.sync_write(state['exp_avg'])
                self.offloader.sync_write(state['exp_avg_sq'])


def run_adam(model: torch.nn.Module, nvme_offload: bool, backend: str, prefetch: int, vecio: bool):
    offloader = None
    if nvme_offload:
        offloader = DiskOffloader('.', backend=backend)
    optimizer = Adam(model.parameters(), 1e-3, offloader=offloader, prefetch=prefetch, vecio=vecio)
    for p in model.parameters():
        p.grad = torch.rand_like(p)
    for _ in range(N_WARMUP):
        optimizer.step()
    start = time()
    for _ in range(N_ACTIVATE):
        optimizer.step()
    dur = time() - start
    if not nvme_offload:
        print(f'CPU: time={dur/N_ACTIVATE:.3f}')
    else:
        print(f'NVME offload: backend={backend}, prefetch={prefetch}, vecio={vecio}, time={dur/N_ACTIVATE:.3f}')


if __name__ == '__main__':
    model = gpt2_small().cpu()
    with torch.no_grad():
        # CPU
        run_adam(model, False, 'uring', 0, False)
        run_adam(model, True, 'uring', 0, False)
        run_adam(model, True, 'uring', 0, True)
        run_adam(model, True, 'uring', 1, False)
        run_adam(model, True, 'uring', 1, True)
