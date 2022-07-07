# ColossalAI NVME offload

> This is a demo.

## Dependencies

- [liburing](https://github.com/axboe/liburing)
- [libaio](https://pagure.io/libaiohttps://pagure.io/libaiohttps://pagure.io/libaio)
- [libtorch](https://github.com/pytorch/pytorch)

## Install

```shell
pip install -v --no-cache-dir -e .
```

## How to test

We have C++ test scrpits for `AsyncIO` and `SpaceManager` class. To run the tests:

```shell
export CMAKE_TORCH_PATH=/path/to/libtorch
export CMAKE_URING_PATH=/path/to/liburing
export CMAKE_AIO_PATH=/path/to/libaio
mkdir build
cd build
cmake ..
make
./test_asyncio
./test_space_mgr
```

Currently, we can save and load Pytorch Tensor:

```python
import torch
from colo_nvme import Offloader
x = torch.rand(2, 2)
print(x)
of = Offloader('test.pth', 4)
of.write(x, str(id(x)))
of.synchronize()
x.zero_()
print(x)
of.read(x, str(id(x)))
of.synchronize()
print(x)
```

A `t.pth` will be generated to save the tensor.
