#pragma once

#include <liburing.h>
#include <unistd.h>
#include <functional>

using callback_t = std::function<void()>;

enum IOType
{
    WRITE,
    READ
};

struct IOData
{
    iovec iov;
    IOType type;
    callback_t callback;

    IOData(IOType type);
    IOData(IOType type, callback_t callback);
};

class AsyncIO
{
public:
    AsyncIO(unsigned int n_entries);
    ~AsyncIO();

    void write(int fd, void *buffer, size_t n_bytes, unsigned long long offset, callback_t callback);
    void read(int fd, void *buffer, size_t n_bytes, unsigned long long offset, callback_t callback);

    void sync_write_events();
    void sync_read_events();
    void synchronize();

private:
    unsigned int n_write_events, n_read_events;
    unsigned int n_entries;
    io_uring ring;
    void wait();
};