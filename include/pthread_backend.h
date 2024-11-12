#pragma once

#include <stdexcept>
#include <sys/io.h>
#include <sys/uio.h>
#include <unistd.h>
#include <cstdlib>
#include <future>
#include <queue>
#include <tuple>
#include <functional>
#include <iostream>
#include <c10/cuda/CUDAStream.h>
#include <c10/cuda/CUDAGuard.h>
#include <atomic>
#include <condition_variable>
#include <mutex>

#include "asyncio.h"
#include "threadpool.hpp"


class PthreadAsyncIO : public AsyncIO
{
private:
    BS::thread_pool pool;
    std::atomic<unsigned int> h2d_in_progress;
    std::condition_variable cv;
    std::mutex mtx;
    std::deque<std::tuple<std::future<ssize_t>, callback_t>> write_fut;
    std::deque<std::tuple<std::future<ssize_t>, callback_t>> read_fut;

public:
    PthreadAsyncIO(unsigned int n_entries)
        : pool(n_entries), h2d_in_progress(0) {}

    ~PthreadAsyncIO() {}

    void write(int fd, void *buffer, size_t n_bytes, unsigned long long offset, callback_t callback);
    void read(int fd, void *buffer, size_t n_bytes, unsigned long long offset, callback_t callback);
    void writev(int fd, const iovec *iov, unsigned int iovcnt, unsigned long long offset, callback_t callback);
    void readv(int fd, const iovec *iov, unsigned int iovcnt, unsigned long long offset, callback_t callback);

    void get_event(WaitType wt);
    void sync_write_events();
    void sync_read_events();
    void register_h2d(unsigned int num_tensors);
    void sync_h2d();
    void synchronize();

    void register_file(int fd);

    void write_tensor(int fd, torch::Tensor t, unsigned long long offset, callback_t callback, std::optional<torch::Tensor> pinned);
};