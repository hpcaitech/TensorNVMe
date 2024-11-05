#pragma once

#include <libaio.h>
#include <torch/torch.h>
#include <stdexcept>
#include <memory>
#include "asyncio.h"

class AIOAsyncIO : public AsyncIO
{
private:
    io_context_t io_ctx = nullptr;
    int n_write_events = 0; /* event个数 */
    int n_read_events = 0;
    int max_nr;
    int min_nr = 1;
    struct timespec timeout;

    void get_event(WaitType wt);

public:
    AIOAsyncIO(unsigned int n_entries);
    ~AIOAsyncIO();

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