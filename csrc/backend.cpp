#include "backend.h"
#include <stdexcept>
#include <cerrno>
#include <cstring>
#include <cassert>
#include <memory>

#ifndef DISABLE_URING
#include "uring.h"
#endif
#ifndef DISABLE_AIO
#include "aio.h"
#endif
#ifndef DISABLE_PTHREAD
#include "pthread_backend.h"
#endif

std::unordered_set<std::string> get_backends()
{
    std::unordered_set<std::string> backends;
#ifndef DISABLE_URING
    backends.insert("uring");
#endif
#ifndef DISABLE_AIO
    backends.insert("aio");
#endif
#ifndef DISABLE_PTHREAD
    backends.insert("pthread");
#endif
    return backends;
}

void probe_asyncio(const std::string &backend)
{
    FILE *fp = tmpfile();
    if (!fp)
    {
        printf("Create tmpfile error: %s\n", strerror(errno));
        throw std::runtime_error("uring probe failed\n");
    }
    try
    {
        std::unique_ptr<AsyncIO> aio;
        if (backend == "uring") {
#ifndef DISABLE_URING
            aio.reset(new UringAsyncIO(2));
#else
            throw std::runtime_error("backend uring is not installed\n");
#endif
        } else if (backend == "aio") {
#ifndef DISABLE_AIO
            aio.reset(new AIOAsyncIO(2));
#else
            throw std::runtime_error("backend aio is not installed\n");
#endif
        } else if (backend == "pthread") {
#ifndef DISABLE_PTHREAD
            aio.reset(new PthreadAsyncIO(2));
#else
            throw std::runtime_error("backend pthread is not installed\n");
#endif
        } else {
            throw std::runtime_error("unknown backend");
        }

        int fd = fileno(fp);
        const int n_loop = 5, n_len = 18;

        char text[n_loop][n_len];

        int offset = 0;
        size_t len;
        for (int i = 0; i < n_loop; i++)
        {
            len = n_len;
            aio->write(fd, text[i], len, offset, nullptr);
            offset += len;
        }
        aio->sync_write_events();

        char new_text[n_loop][n_len];
        offset = 0;
        for (int i = 0; i < n_loop; i++)
        {
            len = n_len;
            aio->read(fd, new_text[i], len, offset, nullptr);
            offset += len;
        }
        aio->sync_read_events();
        for (int i = 0; i < n_loop; i++)
        {
            for (int j = 0; j < n_len; j++)
            {
                assert(text[i][j] == new_text[i][j]);
            }
        }
        fclose(fp);
    }
    catch (...)
    {
        fclose(fp);
        throw std::runtime_error("uring probe failed\n");
    }
}

bool probe_backend(const std::string &backend)
{
    std::unordered_set<std::string> backends = get_backends();
    if (backends.find(backend) == backends.end())
        return false;
    try
    {
        probe_asyncio(backend);
        return true;
    }
    catch (...)
    {
        return false;
    }
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
#ifndef DISABLE_PTHREAD
    if (backend == "pthread")
        return new PthreadAsyncIO(n_entries);
#endif
    throw std::runtime_error("Unsupported backend: " + backend);
}