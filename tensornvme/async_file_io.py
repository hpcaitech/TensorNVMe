import ctypes
from io import IOBase

from tensornvme._C import AsyncFileWriter as AsyncFileWriterC


class AsyncFileWriter:
    def __init__(self, fp: IOBase, n_entries: int = 16) -> None:
        fd = fp.fileno()
        self.io = AsyncFileWriterC(fd, n_entries)
        self.fp = fp
        self.offset = 0
        # must ensure the data is not garbage collected
        self.buffers = []

    def write(self, data: bytes) -> int:
        if isinstance(data, memoryview):
            data = data.tobytes()
        ptr = ctypes.cast(data, ctypes.POINTER(ctypes.c_char))
        addr = ctypes.addressof(ptr.contents)
        self.io.write(addr, len(data), self.offset)
        self.offset += len(data)
        self.buffers.append(data)
        return len(data)

    def flush(self) -> None:
        pass

    def synchronize(self) -> None:
        self.io.synchronize()
        self.buffers.clear()

    def __del__(self) -> None:
        self.synchronize()
