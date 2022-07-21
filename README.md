# TensorNVME

A Python Library provides APIs to move PyTorch Tensors between CPU and NVMe.

## Dependencies

- [liburing](https://github.com/axboe/liburing)
- [libaio](https://pagure.io/libaio)

## Install

This package is only supported on Linux. `liburing` and `libaio` can be automatically installed. `liburing` is supported on Linux >= `5.10`, and it won't be installed if the version of your Linux < `5.10`.

> You must install pytorch and cmake before installing tensornvme. Once you upgrade pytorch, remember to reinstall tensornvme.

First, install requirements:
```shell
pip install -r requirements.txt
```

To install `tensornvme` with `liburing` and `libaio`:
```shell
pip install -v --no-cache-dir .
```

To install `tensornvme` with only `liburing`:
```shell
DISABLE_AIO=1 pip install -v --no-cache-dir .
```

To install `tensornvme` with only `libaio`:
```shell
DISABLE_URING=1 pip install -v --no-cache-dir .
```

It will search `libaio` and `liburing` in `/usr/lib`, `/usr/lib64` and `$LD_LIBRARY_PATH`. If not found, backends will be installed in `~/.tensornvme`, and `~/.bashrc` will be modified to set `$LD_LIBRARY_PATH` correctly. If you use other shells, please make sure `$LD_LIBRARY_PATH` is set correctly.

If you want to install `libaio` or `liburing` for system:
```shell
WITH_ROOT=1 sudo -v --no-cache-dir .
```
Then they will be installed in `/usr` and `~/.bashrc` will not be modified.

## CLI

We provide a CLI to test whether backends work well.
```shell
tensornvme check
```

## Usage

It provide both synchronize and asynchronize I/O API.

> Only CPU and contiguous tensors can be offloaded.

Synchronize API:
```python
import torch
from tensornvme import DiskOffloader

x = torch.rand(2, 2)
y = torch.rand(4, 4, 4)
offloader = DiskOffloader('./offload')
offloader.sync_write(x)
# x is saved to a file on disk (in ./offload folder) and the memory of x is freed
offloader.sync_read(x)
# x is restored
offloader.sync_writev([x, y])
# x and y are offloaded
offloader.sync_readv([x, y])
# x and y are restored.
# sync_writev() and sync_readv() are order sensitive
# E.g. sync_writev([x, y]) and sync_writev([y, x]) are different
```

Asynchronize API:
```python
import torch
from tensornvme import DiskOffloader

x = torch.rand(2, 2)
y = torch.rand(4, 4, 4)
offloader = DiskOffloader('./offload')
offloader.async_write(x)
# x is being offloaded in the background
offloader.sync_write_events()
# x is offloaded and the memory of x is freed
offloader.async_read(x)
# x is being restored in the background
offloader.sync_read_events()
# x is restored
offloader.async_writev([x, y])
# x and y are being offloaded in the background
offloader.synchronize()
# synchronize() will synchronize both write and read events.
offloader.async_readv([x, y])
offloader.synchronize()
# x and y are restored.
# async_writev() and async_readv() are also order sensitive
```

You can use asynchronize API to overlap computation and data moving.
```python
tensors = []

for _ in range(10):
    tensor = torch.rand(2, 2)
    tensors.append(tensor)
    offloader.sync_write(tensor)

offloader.sync_read(tensors[0])

for i, tensor in enumerate(tensors):
    offloader.sync_read_events()
    if i + 1 < len(tensors):
        offloader.async_read(tensors[i+1])
    tensor.mul_(2.0)
    # compute with tensor
    offloader.sync_write_events()
    offloader.async_write(tensor)
offloader.synchronize()
```

## How to test

We have C++ test scrpits for `AsyncIO` and `SpaceManager` class. Make sure you have installed `liburing` and `libaio`, and set environment variables correctly before testing. To run the tests:

```shell
mkdir build
cd build
cmake ..
make
./test_asyncio
./test_space_mgr
```

We also have python unit tests. Make sure you have installed `pytest`. To run:

```shell
pytest ./tests
```
