#include "async_file_io.h"
#include "backend.h"
#include <stdexcept>
#include <string>

AsyncFileIO::AsyncFileIO(int fd, unsigned int n_entries) : fd(fd)
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

void AsyncFileIO::write(void *buffer, size_t n_bytes, unsigned long long offset)
{
    this->aio->write(this->fd, buffer, n_bytes, offset, nullptr);
}

void AsyncFileIO::read(void *buffer, size_t n_bytes, unsigned long long offset)
{
    this->aio->read(this->fd, buffer, n_bytes, offset, nullptr);
}

void AsyncFileIO::synchronize()
{
    this->aio->synchronize();
}

AsyncFileIO::~AsyncFileIO()
{
    delete this->aio;
}