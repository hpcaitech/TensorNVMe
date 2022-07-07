#pragma once

#include <libaio.h>
#include "asyncio.h"

class AIOAsyncIO : public AsyncIO
{
private:
    io_context_t io_ctx = nullptr;
    int n_write_events = 0; /* event个数 */
    int n_read_events = 0;
    int max_nr = 10;
    int min_nr = 1;
    struct timespec timeout
    {
    };

    void wait();

public:
    AIOAsyncIO(unsigned int n_entries);
    ~AIOAsyncIO();

    void write(int fd, void *buffer, size_t n_bytes, unsigned long long offset, callback_t callback);
    void read(int fd, void *buffer, size_t n_bytes, unsigned long long offset, callback_t callback);
    void writev(int fd, const iovec *iov, unsigned int iovcnt, unsigned long long offset, callback_t callback);
    void readv(int fd, const iovec *iov, unsigned int iovcnt, unsigned long long offset, callback_t callback);

    void sync_write_events();
    void sync_read_events();
    void synchronize();

    void register_file(int fd);
};