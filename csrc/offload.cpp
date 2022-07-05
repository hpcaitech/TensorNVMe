#include <stdio.h>
#include <ATen/ATen.h>
#include <torch/extension.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string>
#include <stdexcept>
#include <unordered_map>
#include <error.h>
#include <pybind11/functional.h>
#include "uring.h"
#include "aio.h"
#include "space_mgr.h"

class Offloader
{
public:
    Offloader(const std::string &filename, unsigned int n_entries, const std::string &backend = "uring") : filename(filename), space_mgr(SpaceManager(0))
    {
        if (backend == "uring")
            this->aio = new UringAsyncIO(n_entries);
        else if (backend == "aio")
            this->aio = new AIOAsyncIO(n_entries);
        else
            throw std::runtime_error("Unknown backend");
        this->fd = open(filename.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
        this->aio->register_file(fd);
    }

    SpaceInfo prepare_write(const at::Tensor &tensor, const std::string &key)
    {
        if (!tensor.is_contiguous() || !tensor.is_cpu())
            throw std::runtime_error("Tensor must be contiguous and on cpu");
        ull bytes = tensor.storage().nbytes();
        ull offset = this->space_mgr.alloc(bytes);
        SpaceInfo space_info = SpaceInfo(offset, bytes);
        this->tensors_info[key] = space_info;
        return space_info;
    }

    SpaceInfo prepare_read(const at::Tensor &tensor, const std::string &key)
    {
        if (!tensor.is_contiguous() || !tensor.is_cpu())
            throw std::runtime_error("Tensor must be contiguous and on cpu");
        if (this->tensors_info.find(key) == this->tensors_info.end())
            throw std::runtime_error("Read error, tensor not found");
        ull bytes = tensor.storage().nbytes();
        SpaceInfo space_info = this->tensors_info[key];
        if (bytes != space_info.second)
            throw std::runtime_error("Read error, tensor shape mismatch");
        this->tensors_info.erase(key);
        return space_info;
    }

    void async_write(const at::Tensor &tensor, const std::string &key, callback_t callback = nullptr)
    {
        ull offset, bytes;
        std::tie(offset, bytes) = prepare_write(tensor, key);
        this->aio->write(this->fd, tensor.data_ptr(), bytes, offset, callback);
    }

    void async_read(const at::Tensor &tensor, const std::string &key, callback_t callback = nullptr)
    {
        ull offset, bytes;
        std::tie(offset, bytes) = prepare_read(tensor, key);
        auto fn = std::bind(&Offloader::release, this, offset, bytes, callback);
        this->aio->read(this->fd, tensor.data_ptr(), bytes, offset, fn);
    }

    void sync_write(const at::Tensor &tensor, const std::string &key)
    {
        ull offset, bytes;
        std::tie(offset, bytes) = prepare_write(tensor, key);
        lseek(this->fd, offset, SEEK_SET);
        write(this->fd, tensor.data_ptr(), bytes);
    }

    void sync_read(const at::Tensor &tensor, const std::string &key)
    {
        ull offset, bytes;
        std::tie(offset, bytes) = prepare_read(tensor, key);
        lseek(this->fd, offset, SEEK_SET);
        read(this->fd, tensor.data_ptr(), bytes);
        release(offset, bytes);
    }

    void sync_write_events()
    {
        this->aio->sync_write_events();
    }

    void sync_read_events()
    {
        this->aio->sync_read_events();
    }

    void synchronize()
    {
        this->aio->synchronize();
    }

    ~Offloader()
    {
        errno = 0;
        delete this->aio;
        close(this->fd);
        if (remove(this->filename.c_str()) != 0)
            printf("Remove \"%s\" error(%d): %s\n", this->filename.c_str(), errno, strerror(errno));
    }

private:
    const std::string filename;
    int fd;
    AsyncIO *aio;
    SpaceManager space_mgr;
    std::unordered_map<std::string, SpaceInfo> tensors_info;

    void release(ull offset, ull bytes, callback_t callback = nullptr)
    {
        this->space_mgr.free(offset, bytes);
        if (callback != nullptr)
            callback();
    }
};

PYBIND11_MODULE(TORCH_EXTENSION_NAME, m)
{
    py::class_<Offloader>(m, "Offloader")
        .def(py::init<const std::string &, unsigned int, const std::string &>(), py::arg("filename"), py::arg("n_entries"), py::arg("backend") = "uring")
        .def("async_write", &Offloader::async_write, py::arg("tensor"), py::arg("key"), py::arg("callback") = py::none())
        .def("async_read", &Offloader::async_read, py::arg("tensor"), py::arg("key"), py::arg("callback") = py::none())
        .def("sync_write", &Offloader::sync_write, py::arg("tensor"), py::arg("key"))
        .def("sync_read", &Offloader::sync_read, py::arg("tensor"), py::arg("key"))
        .def("sync_write_events", &Offloader::sync_write_events)
        .def("sync_read_events", &Offloader::sync_write_events)
        .def("synchronize", &Offloader::synchronize);
}