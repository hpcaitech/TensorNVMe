#pragma once

#include <liburing.h>
#include "asyncio.h"

class UringAsyncIO : public AsyncIO
{
private:
    unsigned int n_write_events, n_read_events;
    unsigned int n_entries;
    io_uring ring;

    void get_event(WaitType wt);

public:
    UringAsyncIO(unsigned int n_entries);
    ~UringAsyncIO();

    void write(int fd, void *buffer, size_t n_bytes, unsigned long long offset, callback_t callback);
    void read(int fd, void *buffer, size_t n_bytes, unsigned long long offset, callback_t callback);
    void writev(int fd, const iovec *iov, unsigned int iovcnt, unsigned long long offset, callback_t callback);
    void readv(int fd, const iovec *iov, unsigned int iovcnt, unsigned long long offset, callback_t callback);

    void register_h2d(unsigned int num_tensors);
    void sync_h2d();
    void sync_write_events();
    void sync_read_events();
    void synchronize();

    void register_file(int fd);
    void write_tensor(int fd, torch::Tensor t, unsigned long long offset, callback_t callback, std::optional<torch::Tensor> pinned);
};