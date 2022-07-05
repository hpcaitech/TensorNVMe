#pragma once

#include <liburing.h>
#include "asyncio.h"

class UringAsyncIO : public AsyncIO
{
private:
    unsigned int n_write_events, n_read_events;
    unsigned int n_entries;
    io_uring ring;

    void wait();

public:
    UringAsyncIO(unsigned int n_entries);
    ~UringAsyncIO();

    void write(int fd, void *buffer, size_t n_bytes, unsigned long long offset, callback_t callback);
    void read(int fd, void *buffer, size_t n_bytes, unsigned long long offset, callback_t callback);

    void sync_write_events();
    void sync_read_events();
    void synchronize();

    void register_file(int fd);
};