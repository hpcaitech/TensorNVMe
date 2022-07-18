# ColossalAI NVME offload

> This is a demo.

## Install

To install `colo_nvme` :

```shell
export CMAKE_TORCH_PATH=/path/to/torch
pip install -v --no-cache-dir -e .
```

## How to test

We have C++ test scrpits for `AsyncIO` and `SpaceManager` class. Make sure you have installed `liburing` and `libaio`, and set environment variables correctly before testing. To run the tests:

```shell
mkdir build
cd build
cmake ..
make
./test_asyncio
./test_offload
./test_space_mgr
```

We also have python unit tests. Make sure you have installed `pytest`. To run:

```shell
pytest ./tests
```