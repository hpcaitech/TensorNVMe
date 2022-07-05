import torch
from colo_nvme import DiskOffloader
from titans.model.gpt import gpt2_small
from time import time
from typing import List
from functools import partial


def adam(p, g, m, v, lr=1e-3):
    m.mul_(0.9).add_(g, alpha=0.1)
    v.mul_(0.99).add_(g * g, alpha=0.01)
    denom = v.sqrt().add_(1e-6).mul_(-lr)
    p.addcdiv_(m, denom)


def run_cpu_step(params: List[torch.Tensor], ml: List[torch.Tensor], vl: List[torch.Tensor]):
    start = time()
    for p, m, v in zip(params, ml, vl):
        adam(p, p.grad, m, v)
    return time() - start


def run_nvme_sync(params: List[torch.Tensor], ml: List[torch.Tensor], vl: List[torch.Tensor], of: DiskOffloader):
    start = time()
    for p, m, v in zip(params, ml, vl):
        of.sync_read(m)
        of.sync_read(v)
        adam(p, p.grad, m, v)
        of.sync_write(m)
        of.sync_write(v)
    return time() - start


def run_nvme_async(params: List[torch.Tensor], ml: List[torch.Tensor], vl: List[torch.Tensor], of: DiskOffloader):
    start = time()
    of.async_read(ml[0])
    of.async_read(vl[0])

    for i, (p, m, v) in enumerate(zip(params, ml, vl)):
        of.sync_read_events()
        if i + 1 < len(params):
            of.async_read(ml[i + 1])
            of.async_read(vl[i + 1])
        adam(p, p.grad, m, v)
        of.sync_write_events()
        of.async_write(m)
        of.async_write(v)

    of.synchronize()
    return time() - start


def timeit(fn, n_warmup, n_iter):
    for _ in range(n_warmup):
        fn()
    total_time = 0.0
    for _ in range(n_iter):
        total_time += fn()
    return total_time / n_iter


if __name__ == '__main__':
    N_WARMUP = 5
    N_RUN = 10
    model = gpt2_small().cpu()
    params = []
    momentums = []
    variances = []
    for p in model.parameters():
        p.grad = torch.rand_like(p)
        params.append(p)
        momentums.append(torch.zeros_like(p))
        variances.append(torch.zeros_like(p))
    of = DiskOffloader('.')
    with torch.no_grad():
        cpu_time = timeit(partial(run_cpu_step, params, momentums, variances), N_WARMUP, N_RUN)
        print(f'CPU={cpu_time:.3f}')
        for m, v in zip(momentums, variances):
            of.async_write(m)
            of.async_write(v)
        of.synchronize()
        nvme_sync_time = timeit(partial(run_nvme_sync, params, momentums, variances, of), N_WARMUP, N_RUN)
        print(f'NVME Sync={nvme_sync_time:.3f}')
        nvme_async_time = timeit(partial(run_nvme_async, params, momentums, variances, of), N_WARMUP, N_RUN)
        print(f'NVME Async={nvme_async_time:.3f}')
