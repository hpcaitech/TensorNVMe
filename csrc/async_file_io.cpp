#include "async_file_io.h"

AsyncFileWriter::AsyncFileWriter(int fd, unsigned int n_entries, const std::string &backend) : fd(fd), aio(create_asyncio(n_entries, backend)) {}

void AsyncFileWriter::write(size_t buffer, size_t n_bytes, unsigned long long offset, callback_t callback)
{
    void *ptr = reinterpret_cast<void *>(buffer);
    this->aio->write(this->fd, ptr, n_bytes, offset, callback);
}

void AsyncFileWriter::write_tensor(torch::Tensor tensor, unsigned long long offset, callback_t callback, std::optional<torch::Tensor> pinned)
{
    this->aio->write_tensor(this->fd, tensor, offset, callback, pinned);
}

void AsyncFileWriter::register_h2d(unsigned int num_tensors)
{
    this->aio->register_h2d(num_tensors);
}

void AsyncFileWriter::sync_h2d()
{
    this->aio->sync_h2d();
}

void AsyncFileWriter::synchronize()
{
    this->aio->synchronize();
}

AsyncFileWriter::~AsyncFileWriter()
{
    delete this->aio;
}

void AsyncFileWriter::register_tasks(unsigned int num_tasks)
{
    this->aio->register_tasks(num_tasks);
}