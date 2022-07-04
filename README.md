# ColossalAI NVME offload

> This is a demo.

## Dependencies
- [liburing](https://github.com/axboe/liburing)

## Install
```shell
pip install -v --no-cache-dir -e .
```

## How to test
`tests/test_asyncio.cpp` is a simple demo to test `AsyncIO` class. To compile:

```shell
g++ tests/test_asyncio.cpp csrc/aio.cpp -luring
./a.out
```

You will get a `test.txt` which is filled with `"TEST ME AGAIN!!!"`.

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
