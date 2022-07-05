import os
import torch
import uuid
from typing import Callable, Optional
from functools import partial
from colo_nvme._C import Offloader


class DiskOffloader(Offloader):
    def __init__(self, dir_name: str, n_entries: int = 64, backend: str = 'uring') -> None:
        assert backend in ('uring', 'aio')
        if not os.path.exists(dir_name):
            os.mkdir(dir_name)
        assert os.path.isdir(dir_name)
        filename = os.path.join(dir_name, f'offload-{uuid.uuid4().hex}')
        while os.path.exists(filename):
            filename = os.path.join(dir_name, f'offload-{uuid.uuid4().hex}')
        super().__init__(filename, n_entries, backend)

    def write(self, tensor: torch.Tensor, callback: Optional[Callable[[], None]] = None) -> None:
        assert tensor.storage().size() > 0
        super().write(tensor, str(id(tensor)), partial(DiskOffloader._write_callback, tensor, callback))

    def read(self, tensor: torch.Tensor, callback: Optional[Callable[[], None]] = None) -> None:
        if tensor.storage().size() == 0:
            tensor.storage().resize_(tensor.numel())
        super().read(tensor, str(id(tensor)), callback)

    @staticmethod
    def _write_callback(tensor: torch.Tensor, callback: Optional[Callable[[], None]] = None) -> None:
        tensor.storage().resize_(0)
        if callback is not None:
            callback()
