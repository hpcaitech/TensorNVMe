#include <stdio.h>
#include <ATen/ATen.h>
#include <torch/extension.h>
#include <torch/csrc/utils/pybind.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string>
#include <stdexcept>
#include "aio.h"

class Offloader
{
    int fd;
    AsyncIO aio;

public:
    Offloader(const char *filename, unsigned int n_entries) : aio(AsyncIO(n_entries))
    {
        this->fd = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    }

    void write(const at::Tensor &tensor)
    {
        if (!tensor.is_contiguous() || !tensor.is_cpu())
            throw std::runtime_error("Tensor must be contiguous and on cpu");
        this->aio.write(this->fd, tensor.data_ptr(), tensor.storage().nbytes(), 0);
        this->aio.sync_write_events();
    }

    void read(const at::Tensor &tensor)
    {
        if (!tensor.is_contiguous() || !tensor.is_cpu())
            throw std::runtime_error("Tensor must be contiguous and on cpu");
        this->aio.read(this->fd, tensor.data_ptr(), tensor.storage().nbytes(), 0);
        this->aio.sync_read_events();
    }
};

void writet(const at::Tensor &tensor)
{
    if (!tensor.is_contiguous() || !tensor.is_cpu())
        throw std::runtime_error("Tensor must be contiguous and on cpu");
    AsyncIO aio(2);
    int fd = open("t.pth", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    aio.write(fd, tensor.data_ptr(), tensor.storage().nbytes(), 0);
    aio.sync_write_events();
    close(fd);
}
void readt(const at::Tensor &tensor)
{
    if (!tensor.is_contiguous() || !tensor.is_cpu())
        throw std::runtime_error("Tensor must be contiguous and on cpu");
    AsyncIO aio(2);
    int fd = open("t.pth", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    aio.read(fd, tensor.data_ptr(), tensor.storage().nbytes(), 0);
    aio.sync_read_events();
    close(fd);
}

PYBIND11_MODULE(colo_nvme, m)
{
    m.def("read", &readt, "read");
    m.def("write", &writet, "write");
}