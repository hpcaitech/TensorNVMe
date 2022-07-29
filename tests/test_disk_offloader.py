import torch
import pytest
from tensornvme import DiskOffloader


@pytest.mark.parametrize('backend', ['uring', 'aio'])
def test_sync_io(backend):
    x = torch.rand(2, 2)
    x_copy = x.clone()
    of = DiskOffloader('.', backend=backend)
    try:
        of.sync_read(x)
        assert False
    except RuntimeError:
        pass
    of.sync_write(x)
    assert x.storage().size() == 0
    of.sync_read(x)
    assert torch.equal(x, x_copy)


@pytest.mark.parametrize('backend', ['uring', 'aio'])
def test_async_io(backend):
    x = torch.rand(2, 2)
    x_copy = x.clone()
    of = DiskOffloader('.', backend=backend)
    try:
        of.async_read(x)
        assert False
    except RuntimeError:
        pass
    of.async_write(x)
    # assert x.storage().size() > 0
    of.sync_write_events()
    assert x.storage().size() == 0
    of.sync_read(x)
    of.sync_read_events()
    assert torch.equal(x, x_copy)


@pytest.mark.parametrize('backend', ['uring', 'aio'])
def test_sync_vec_io(backend):
    x = torch.rand(2, 2)
    y = torch.rand(2, 2, 2)
    x_copy = x.clone()
    y_copy = y.clone()
    of = DiskOffloader('.', backend=backend)
    try:
        of.sync_readv([x, y])
        assert False
    except RuntimeError:
        pass
    of.sync_writev([x, y])
    assert x.storage().size() == 0
    assert y.storage().size() == 0
    try:
        of.sync_readv(x)
        assert False
    except RuntimeError:
        pass
    try:
        of.sync_readv([y, x])
        assert False
    except RuntimeError:
        pass
    of.sync_readv([x, y])
    assert torch.equal(x, x_copy)
    assert torch.equal(y, y_copy)


@pytest.mark.parametrize('backend', ['uring', 'aio'])
def test_async_vec_io(backend):
    x = torch.rand(2, 2)
    y = torch.rand(2, 2, 2)
    x_copy = x.clone()
    y_copy = y.clone()
    of = DiskOffloader('.', backend=backend)
    try:
        of.async_readv([x, y])
        assert False
    except RuntimeError:
        pass
    of.async_writev([x, y])
    # assert x.storage().size() > 0
    # assert y.storage().size() > 0
    of.sync_write_events()
    assert x.storage().size() == 0
    assert y.storage().size() == 0
    try:
        of.async_readv(x)
        assert False
    except RuntimeError:
        pass
    try:
        of.async_readv([y, x])
        assert False
    except RuntimeError:
        pass
    of.async_readv([x, y])
    of.sync_read_events()
    assert torch.equal(x, x_copy)
    assert torch.equal(y, y_copy)


if __name__ == '__main__':
    test_sync_io('uring')
    test_async_io('uring')
    test_sync_vec_io('uring')
    test_async_vec_io('uring')
