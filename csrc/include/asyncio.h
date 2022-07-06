#pragma once

#include <fcntl.h>
#include <functional>

using callback_t = std::function<void()>;

enum IOType
{
    WRITE,
    READ
};

struct IOData
{
    IOType type;
    callback_t callback;

    IOData(IOType type) : type(type), callback(nullptr) {}
    IOData(IOType type, callback_t callback) : type(type), callback(callback) {}
};

class AsyncIO
{
public:
    virtual ~AsyncIO() = default;

    virtual void write(int fd, void *buffer, size_t n_bytes, unsigned long long offset, callback_t callback) = 0;
    virtual void read(int fd, void *buffer, size_t n_bytes, unsigned long long offset, callback_t callback) = 0;

    virtual void sync_write_events() = 0;
    virtual void sync_read_events() = 0;
    virtual void synchronize() = 0;

    virtual void register_file(int fd) = 0;
};