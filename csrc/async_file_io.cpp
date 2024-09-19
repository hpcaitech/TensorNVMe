#include "async_file_io.h"
#include "backend.h"
#include <stdexcept>
#include <string>

AsyncFileWriter::AsyncFileWriter(int fd, unsigned int n_entries) : fd(fd)
{
    for (const std::string &backend : get_backends())
    {
        if (probe_backend(backend))
        {
            this->aio = create_asyncio(n_entries, backend);
            return;
        }
    }
    throw std::runtime_error("No asyncio backend is installed");
}

void AsyncFileWriter::write(size_t buffer, size_t n_bytes, unsigned long long offset)
{
    void *ptr = reinterpret_cast<void *>(buffer);
    this->aio->write(this->fd, ptr, n_bytes, offset, nullptr);
}

void AsyncFileWriter::synchronize()
{
    this->aio->synchronize();
}

AsyncFileWriter::~AsyncFileWriter()
{
    delete this->aio;
}