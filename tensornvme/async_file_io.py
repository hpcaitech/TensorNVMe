import ctypes
from functools import partial
import torch

from typing import Dict, List
from io import IOBase
from tensornvme._C import AsyncFileWriter as AsyncFileWriterC

from colossalai.utils.safetensors import prepare

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

    def save(
        self,
        state_dict: Dict[str, torch.Tensor]
    ) -> None:
        prepared_data, tensors = prepare(state_dict)
        n, header_bytes, _ = prepared_data.n, prepared_data.header_bytes, prepared_data.offset

        self.write(n.to_bytes(8, byteorder='little'))
        self.write(header_bytes)

        for tensor in tensors:
            self.write_raw(tensor, tensor.data_ptr(), tensor.numel() * tensor.element_size(), self.offset)

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
