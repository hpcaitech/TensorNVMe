import ctypes
import torch
from functools import partial
from torch import Tensor
from typing import List, Optional
from io import IOBase
from tensornvme._C import AsyncFileWriter as AsyncFileWriterC

class AsyncFileWriter:
    def __init__(self, fp: IOBase, n_entries: int = 16, backend=None) -> None:
        fd = fp.fileno()
        if backend is not None:
            self.io = AsyncFileWriterC(fd, n_entries, backend=backend)
        else:
            self.io = AsyncFileWriterC(fd, n_entries)
        self.fp = fp
        self.offset = 0
        # must ensure the data is not garbage collected
        self.buffers = []
        self.comm_stream = torch.cuda.Stream()

    def write(self, data: bytes) -> int:
        ptr = ctypes.cast(data, ctypes.POINTER(ctypes.c_char))
        addr = ctypes.addressof(ptr.contents)
        self.buffers.append(data)  # append before callback is called
        self.io.write(addr, len(data), self.offset, partial(AsyncFileWriter.gc_callback, self.buffers, len(self.buffers) - 1))
        self.offset += len(data)

        return len(data)

    def write_raw(self, py_ref: object, buffer: int, n_bytes: int, offset: int) -> None:
        self.buffers.append(py_ref)  # append before callback is called
        self.io.write(buffer, n_bytes, offset, partial(AsyncFileWriter.gc_callback, self.buffers, len(self.buffers) - 1))
        self.offset += n_bytes

    def write_tensor(self, tensor: Tensor, pinned: Optional[Tensor] = None) -> None:
        self.buffers.append(tensor)  # append before callback is called
        self.io.write_tensor(tensor, self.offset, partial(AsyncFileWriter.gc_callback, self.buffers, len(self.buffers) - 1), pinned)
        self.offset += tensor.numel() * tensor.element_size()

    def write_gpu_tensor(self, tensor: Tensor, pinned: Optional[Tensor] = None) -> None:
        assert tensor.device.type == 'cuda', f"tensor must be on cuda device, got {tensor.device}"
        with torch.cuda.stream(self.comm_stream):
            self.write_tensor(tensor, pinned)

    def sync_before_step(self):
        self.comm_stream.synchronize()

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
