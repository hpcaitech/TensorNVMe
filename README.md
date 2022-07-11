# ColossalAI NVME offload

> This is a demo.

## Dependencies

- [liburing](https://github.com/axboe/liburing)
- [libaio](https://pagure.io/libaio)

## Install

You must install `liburing` and `libaio` first. You can install them through your package manager or from source code. We will introduce you how to install them from source code.

### Install liburing
```shell
git clone https://github.com/axboe/liburing.git
cd liburing
# If you have sudo privilege
./configure && make && sudo make install
# If you don't have sudo privilege
./configure --prefix=~/.local && make && make install
# "~/.local" can be replaced with any path
```

### Install libaio
```shell
git clone https://pagure.io/libaio.git
cd libaio
# If you have sudo privilege
sudo make prefix=/usr install
# If you don't have sudo privilege
make prefix=~/.local install
```

If you install `liburing` or `libaio` without `sudo`, you must set environment variables correctly. Here is an example code snippet in `~/.bashrc`:
```shell
export LIBRARY_PATH=$HOME/.local/lib:$LIBRARY_PATH
export LD_LIBRARY_PATH=$HOME/.local/lib:$LD_LIBRARY_PATH
export C_INCLUDE_PATH=$HOME/.local/include:$C_INCLUDE_PATH
export CPLUS_INCLUDE_PATH=$HOME/.local/include:$CPLUS_INCLUDE_PATH
```

### Install colo_nvme

To install `colo_nvme` with `liburing` and `libaio`:
```shell
pip install -v --no-cache-dir -e .
```

To install `colo_nvme` with only `liburing`:
```shell
DISABLE_AIO=1 pip install -v --no-cache-dir -e .
```

To install `colo_nvme` with only `libaio`:
```shell
DISABLE_URING=1 pip install -v --no-cache-dir -e .
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