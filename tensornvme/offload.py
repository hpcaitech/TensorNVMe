import os
import torch
import uuid
from typing import Callable, Optional, List
from tensornvme._C import Offloader, get_backends


class DiskOffloader(Offloader):
    def __init__(self, dir_name: str, n_entries: int = 16, backend: str = 'uring') -> None:
        assert backend in get_backends(
        ), f'Unsupported backend: {backend}, please install tensornvme with this backend'
        if not os.path.exists(dir_name):
            os.mkdir(dir_name)
        assert os.path.isdir(dir_name)
        filename = os.path.join(dir_name, f'offload-{uuid.uuid4().hex}')
        while os.path.exists(filename):
            filename = os.path.join(dir_name, f'offload-{uuid.uuid4().hex}')
        super().__init__(filename, n_entries, backend)

    def async_write(self, tensor: torch.Tensor, callback: Optional[Callable[[], None]] = None) -> None:
        assert tensor.storage().size() > 0

        def callback_fn():
            tensor.storage().resize_(0)
            if callback is not None:
                callback()
        super().async_write(tensor, str(id(tensor)), callback_fn)

    def async_read(self, tensor: torch.Tensor, callback: Optional[Callable[[], None]] = None) -> None:
        if tensor.storage().size() == 0:
            tensor.storage().resize_(tensor.numel())
        super().async_read(tensor, str(id(tensor)), callback)

    def sync_write(self, tensor: torch.Tensor) -> None:
        assert tensor.storage().size() > 0
        super().sync_write(tensor, str(id(tensor)))
        tensor.storage().resize_(0)

    def sync_read(self, tensor: torch.Tensor) -> None:
        if tensor.storage().size() == 0:
            tensor.storage().resize_(tensor.numel())
        super().sync_read(tensor, str(id(tensor)))

    def async_writev(self, tensors: List[torch.Tensor], callback: Optional[Callable[[], None]] = None) -> None:
        for tensor in tensors:
            assert tensor.storage().size() > 0
        key = str(hash(tuple(tensors)))

        def callback_fn():
            for tensor in tensors:
                tensor.storage().resize_(0)
            if callback is not None:
                callback()
        super().async_writev(tensors, key, callback_fn)

    def async_readv(self, tensors: List[torch.Tensor], callback: Optional[Callable[[], None]] = None) -> None:
        for tensor in tensors:
            if tensor.storage().size() == 0:
                tensor.storage().resize_(tensor.numel())
        key = str(hash(tuple(tensors)))
        super().async_readv(tensors, key, callback)

    def sync_writev(self, tensors: List[torch.Tensor]) -> None:
        for tensor in tensors:
            assert tensor.storage().size() > 0
        key = str(hash(tuple(tensors)))
        super().sync_writev(tensors, key)
        for tensor in tensors:
            tensor.storage().resize_(0)

    def sync_readv(self, tensors: List[torch.Tensor]) -> None:
        for tensor in tensors:
            if tensor.storage().size() == 0:
                tensor.storage().resize_(tensor.numel())
        key = str(hash(tuple(tensors)))
        super().sync_readv(tensors, key)
