#pragma once

#include <stdexcept>
#include <sys/io.h>
#include <cstdlib>
#include <future>
#include <queue>
#include "asyncio.h"
#include "threadpool.hpp"


class PthreadAsyncIO : public AsyncIO
{
private:
    BS::thread_pool pool;
    std::deque<std::future<int>> write_fut;
    std::deque<std::future<int>> read_fut;

public:
    PthreadAsyncIO(unsigned int n_entries)
        : pool(n_entries) {}

    ~PthreadAsyncIO() {}

    void write(int fd, void *buffer, size_t n_bytes, unsigned long long offset, callback_t callback);
    void read(int fd, void *buffer, size_t n_bytes, unsigned long long offset, callback_t callback);
    void writev(int fd, const iovec *iov, unsigned int iovcnt, unsigned long long offset, callback_t callback);
    void readv(int fd, const iovec *iov, unsigned int iovcnt, unsigned long long offset, callback_t callback);

    void get_event(WaitType wt);
    void sync_write_events();
    void sync_read_events();
    void synchronize();

    void register_file(int fd);
};