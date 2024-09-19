#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include "offload.h"
#include "async_file_io.h"
#include "backend.h"
#include <string>

namespace py = pybind11;

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
        .def("synchronize", &Offloader::synchronize)
        .def("async_writev", &Offloader::async_writev, py::arg("tensors"), py::arg("key"), py::arg("callback") = py::none())
        .def("async_readv", &Offloader::async_readv, py::arg("tensors"), py::arg("key"), py::arg("callback") = py::none())
        .def("sync_writev", &Offloader::sync_writev, py::arg("tensors"), py::arg("key"))
        .def("sync_readv", &Offloader::sync_readv, py::arg("tensors"), py::arg("key"));
    m.def("get_backends", get_backends);
    m.def("probe_backend", probe_backend, py::arg("backend"));
    py::class_<AsyncFileWriter>(m, "AsyncFileWriter")
        .def(py::init<int, unsigned int>(), py::arg("fd"), py::arg("n_entries"))
        .def("write", &AsyncFileWriter::write, py::arg("buffer"), py::arg("n_bytes"), py::arg("offset"))
        .def("synchronize", &AsyncFileWriter::synchronize);
}