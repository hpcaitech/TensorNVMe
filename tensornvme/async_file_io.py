import ctypes
import os
from functools import partial
from typing import List, Optional

import torch
from torch import Tensor

from tensornvme._C import AsyncFileWriter as AsyncFileWriterC


class AsyncFileWriter:
    def __init__(self, path: str, n_entries: int = 16, backend=None) -> None:
        # this still takes ram buffer, which may lead to OOM
        # self.f = open(path, "wb", buffering=0)
        self.fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_DIRECT, mode=0o664)
        if backend is not None:
            self.io = AsyncFileWriterC(self.fd, n_entries, backend=backend)
        else:
            self.io = AsyncFileWriterC(self.fd, n_entries)
        self.offset = 0
        # must ensure the data is not garbage collected
        self.buffers = []
        self.comm_stream = torch.cuda.Stream()

    def write(self, data: bytes) -> int:
        ptr = ctypes.cast(data, ctypes.POINTER(ctypes.c_char))
        addr = ctypes.addressof(ptr.contents)
        self.buffers.append(data)  # append before callback is called
        self.io.write(
            addr, len(data), self.offset, partial(AsyncFileWriter.gc_callback, self.buffers, len(self.buffers) - 1)
        )
        self.offset += len(data)

        return len(data)

    def write_raw(self, py_ref: object, buffer: int, n_bytes: int, offset: int) -> None:
        self.buffers.append(py_ref)  # append before callback is called
        self.io.write(
            buffer, n_bytes, offset, partial(AsyncFileWriter.gc_callback, self.buffers, len(self.buffers) - 1)
        )
        self.offset += n_bytes

    def write_tensor(self, tensor: Tensor, pinned: Optional[Tensor] = None) -> None:
        with torch.cuda.stream(self.comm_stream):
            self.buffers.append(tensor)  # append before callback is called
            self.io.write_tensor(
                tensor, self.offset, partial(AsyncFileWriter.gc_callback, self.buffers, len(self.buffers) - 1), pinned
            )
            self.offset += tensor.numel() * tensor.element_size()

    def register_h2d(self, num_tensors: int) -> None:
        self.io.register_h2d(num_tensors)

    def sync_before_step(self):
        self.io.sync_h2d()

    @staticmethod
    def gc_callback(listt: List, idx: int) -> None:
        listt[idx] = None

    def flush(self) -> None:
        pass

    def synchronize(self) -> None:
        self.io.synchronize()
        self.buffers.clear()

    def __del__(self) -> None:
        self.synchronize()
        os.close(self.fd)
