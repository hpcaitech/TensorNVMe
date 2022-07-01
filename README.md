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
import colo_nvme
x = torch.rand(2, 2)
print(x)
colo_nvme.write(x)
x.zero_()
print(x)
colo_nvme.read(x)
print(x)
```

A `t.pth` will be generated to save the tensor.
