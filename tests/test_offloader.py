import torch
from functools import partial
from colo_nvme import Offloader


def test_eq():
    x = torch.rand(2, 2)
    x_copy = x.clone()
    of = Offloader('test.pth', 4)
    of.write(x, str(id(x)))
    of.synchronize()
    x.zero_()
    assert not torch.equal(x, x_copy)
    of.read(x, str(id(x)))
    of.synchronize()
    assert torch.equal(x, x_copy)


def test_callback():
    x = torch.rand(2, 2)
    x_copy = x.clone()
    of = Offloader('test.pth', 4)
    callback = partial(x.storage().resize_, 0)
    of.write(x, str(id(x)), callback)
    of.synchronize()
    assert x.storage().size() == 0
    x.storage().resize_(x.numel())
    of.read(x, str(id(x)))
    assert torch.equal(x, x_copy)


def test_backend():
    try:
        of = Offloader('test.pth', 4, backend='none')
        assert False
    except RuntimeError:
        pass


if __name__ == '__main__':
    test_eq()
    test_callback()
    test_backend()
