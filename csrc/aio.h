#pragma once

#include <liburing.h>
#include <unistd.h>

enum IOType
{
    WRITE,
    READ
};

struct IOData
{
    iovec iov;
    IOType type;
};

class AsyncIO
{
public:
    AsyncIO(unsigned int n_entries);
    ~AsyncIO();

    void write(int fd, void *buffer, size_t n_bytes, unsigned long long offset);
    void read(int fd, void *buffer, size_t n_bytes, unsigned long long offset);

    void sync_write_events();
    void sync_read_events();
    void synchronize();
private:
    unsigned int n_write_events, n_read_events;
    unsigned int n_entries;
    io_uring ring;
    void wait();
};