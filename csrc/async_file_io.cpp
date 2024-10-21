#include "async_file_io.h"

AsyncFileWriter::AsyncFileWriter(int fd, unsigned int n_entries, const std::string &backend) : fd(fd), aio(create_asyncio(n_entries, backend)) {}

void AsyncFileWriter::write(size_t buffer, size_t n_bytes, unsigned long long offset, callback_t callback)
{
    void *ptr = reinterpret_cast<void *>(buffer);
    this->aio->write(this->fd, ptr, n_bytes, offset, callback);
}

void AsyncFileWriter::write_tensor(torch::Tensor tensor, unsigned long long offset, callback_t callback) {
    this->aio->write_tensor(this->fd, tensor, offset, callback);
}


void AsyncFileWriter::synchronize()
{
    this->aio->synchronize();
}

AsyncFileWriter::~AsyncFileWriter()
{
    delete this->aio;
}