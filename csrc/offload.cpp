#include <stdio.h>
#include <ATen/ATen.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <error.h>
#include <pybind11/functional.h>
#include "offload.h"
#include <pybind11/pybind11.h>
#include "space_mgr.h"
#ifndef DISABLE_URING
#include "uring.h"
#endif
#ifndef DISABLE_AIO
#include "aio.h"
#endif

iovec *tensors_to_iovec(const std::vector<at::Tensor> &tensors)
{
    iovec *iovs = static_cast<iovec *>(calloc(tensors.size(), sizeof(iovec)));
    for (size_t i = 0; i < tensors.size(); i++)
    {
        iovs[i].iov_base = tensors[i].data_ptr();
        iovs[i].iov_len = tensors[i].storage().nbytes();
    }
    return iovs;
}

std::unordered_set<std::string> get_backends()
{
    std::unordered_set<std::string> backends;
#ifndef DISABLE_URING
    backends.insert("uring");
#endif
#ifndef DISABLE_AIO
    backends.insert("aio");
#endif
    return backends;
}

bool probe_backend(const std::string &backend)
{
    std::unordered_set<std::string> backends = get_backends();
    if (backends.find(backend) == backends.end())
        return false;
    // TODO: do probe for backend
    return true;
}

AsyncIO *create_asyncio(unsigned int n_entries, const std::string &backend)
{
    std::unordered_set<std::string> backends = get_backends();
    if (backends.empty())
        throw std::runtime_error("No asyncio backend is installed");
    if (backends.find(backend) == backends.end())
        throw std::runtime_error("Unsupported backend: " + backend);
    if (!probe_backend(backend))
        throw std::runtime_error("Backend \"" + backend + "\" is not install correctly");
#ifndef DISABLE_URING
    if (backend == "uring")
        return new UringAsyncIO(n_entries);
#endif
#ifndef DISABLE_AIO
    if (backend == "aio")
        return new AIOAsyncIO(n_entries);
#endif
    throw std::runtime_error("Unsupported backend: " + backend);
}


Offloader::Offloader(const std::string &filename, unsigned int n_entries, const std::string &backend) : filename(filename), space_mgr(SpaceManager(0))
    {
        this->aio = create_asyncio(n_entries, backend);
        this->fd = open(filename.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
        this->aio->register_file(fd);
    }

SpaceInfo Offloader::prepare_write(const at::Tensor &tensor, const std::string &key)
    {
        if (!tensor.is_contiguous() || !tensor.is_cpu())
            throw std::runtime_error("Tensor must be contiguous and on cpu");
        ull bytes = tensor.storage().nbytes();
        ull offset = this->space_mgr.alloc(bytes);
        SpaceInfo space_info(offset, bytes);
        this->tensors_info[key] = space_info;
        return space_info;
    }

SpaceInfo Offloader::prepare_read(const at::Tensor &tensor, const std::string &key)
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

void Offloader::async_write(const at::Tensor &tensor, const std::string &key, callback_t callback)
    {
        ull offset, bytes;
        std::tie(offset, bytes) = prepare_write(tensor, key);
        this->aio->write(this->fd, tensor.data_ptr(), bytes, offset, callback);
    }

void Offloader::async_read(const at::Tensor &tensor, const std::string &key, callback_t callback)
    {
        ull offset, bytes;
        std::tie(offset, bytes) = prepare_read(tensor, key);
        auto fn = std::bind(&Offloader::release, this, offset, bytes, callback);
        this->aio->read(this->fd, tensor.data_ptr(), bytes, offset, fn);
    }

void Offloader::sync_write(const at::Tensor &tensor, const std::string &key)
    {
        ull offset, bytes;
        std::tie(offset, bytes) = prepare_write(tensor, key);
        lseek(this->fd, offset, SEEK_SET);
        write(this->fd, tensor.data_ptr(), bytes);
    }

void Offloader::sync_read(const at::Tensor &tensor, const std::string &key)
{
    ull offset, bytes;
    std::tie(offset, bytes) = prepare_read(tensor, key);
    lseek(this->fd, offset, SEEK_SET);
    read(this->fd, tensor.data_ptr(), bytes);
    release(offset, bytes);
}

void Offloader::sync_write_events()
{
    this->aio->sync_write_events();
}

void Offloader::sync_read_events()
{
    this->aio->sync_read_events();
}

void Offloader::synchronize()
{
    this->aio->synchronize();
}

Offloader::~Offloader()
{
    errno = 0;
    delete this->aio;
    close(this->fd);
    if (remove(this->filename.c_str()) != 0)
        printf("Remove \"%s\" error(%d): %s\n", this->filename.c_str(), errno, strerror(errno));
}

SpaceInfo Offloader::prepare_writev(const std::vector<at::Tensor> &tensors, const std::string &key)
{
    ull total_bytes = 0;
    for (const at::Tensor &tensor : tensors)
    {
        if (!tensor.is_contiguous() || !tensor.is_cpu())
            throw std::runtime_error("Tensor must be contiguous and on cpu");
        total_bytes += tensor.storage().nbytes();
    }
    ull offset = this->space_mgr.alloc(total_bytes);
    SpaceInfo space_info(offset, total_bytes);
    this->tensors_info[key] = space_info;
    return space_info;
}

SpaceInfo Offloader::prepare_readv(const std::vector<at::Tensor> &tensors, const std::string &key)
{
    ull total_bytes = 0;
    for (const at::Tensor &tensor : tensors)
    {
        if (!tensor.is_contiguous() || !tensor.is_cpu())
            throw std::runtime_error("Tensor must be contiguous and on cpu");
        total_bytes += tensor.storage().nbytes();
    }
    if (this->tensors_info.find(key) == this->tensors_info.end())
        throw std::runtime_error("Read error, tensor not found");
    SpaceInfo space_info = this->tensors_info[key];
    if (total_bytes != space_info.second)
        throw std::runtime_error("Read error, tensor shape mismatch");
    this->tensors_info.erase(key);
    return space_info;
}

void Offloader::async_writev(const std::vector<at::Tensor> &tensors, const std::string &key, callback_t callback)
{
    ull offset, bytes;
    std::tie(offset, bytes) = prepare_writev(tensors, key);
    iovec *iov = tensors_to_iovec(tensors);
    this->aio->writev(this->fd, iov, tensors.size(), offset, callback);
}

void Offloader::async_readv(const std::vector<at::Tensor> &tensors, const std::string &key, callback_t callback)
{

    ull offset, bytes;
    std::tie(offset, bytes) = prepare_readv(tensors, key);
    iovec *iov = tensors_to_iovec(tensors);
    auto fn = std::bind(&Offloader::release, this, offset, bytes, callback);
    this->aio->readv(this->fd, iov, tensors.size(), offset, fn);
}

void Offloader::sync_writev(const std::vector<at::Tensor> &tensors, const std::string &key)
{
    ull offset, bytes;
    std::tie(offset, bytes) = prepare_writev(tensors, key);
    iovec *iov = tensors_to_iovec(tensors);
    lseek(this->fd, offset, SEEK_SET);
    writev(this->fd, iov, tensors.size());
    delete iov;
}

void Offloader::sync_readv(const std::vector<at::Tensor> &tensors, const std::string &key)
{
    ull offset, bytes;
    std::tie(offset, bytes) = prepare_readv(tensors, key);
    iovec *iov = tensors_to_iovec(tensors);
    lseek(this->fd, offset, SEEK_SET);
    readv(this->fd, iov, tensors.size());
    delete iov;
}


void Offloader::release(ull offset, ull bytes, callback_t callback)
{
    this->space_mgr.free(offset, bytes);
    if (callback != nullptr)
        callback();
}


namespace py = pybind11;

PYBIND11_MODULE(Offloader, m)
{
    py::class_<Offloader>(m, "Offloader")
        .def(py::init<const std::string &, unsigned int, const std::string &>(), py::arg("filename"), py::arg("n_entries"), py::arg("backend") = "uring")
        .def("async_write", &Offloader::async_write, py::arg("tensor"), py::arg("key"), py::arg("callback") = py::none())
        .def("async_read", &Offloader::async_read, py::arg("tensor"), py::arg("key"), py::arg("callback") = py::none())
        .def("sync_write", &Offloader::sync_write, py::arg("tensor"), py::arg("key"))
        .def("sync_read", &Offloader::sync_read, py::arg("tensor"), py::arg("key"))
        .def("sync_write_events", &Offloader::sync_write_events)
        .def("sync_read_events", &Offloader::sync_write_events)
        .def("synchronize", &Offloader::synchronize)
        .def("async_writev", &Offloader::async_writev, py::arg("tensors"), py::arg("key"), py::arg("callback") = py::none())
        .def("async_readv", &Offloader::async_readv, py::arg("tensors"), py::arg("key"), py::arg("callback") = py::none())
        .def("sync_writev", &Offloader::sync_writev, py::arg("tensors"), py::arg("key"))
        .def("sync_readv", &Offloader::sync_readv, py::arg("tensors"), py::arg("key"));
//    m.def("get_backends", get_backends);
//    m.def("probe_backend", probe_backend, py::arg("backend"));
}