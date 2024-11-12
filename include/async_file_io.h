#pragma once
#include <string>
#include <torch/torch.h>
#include <optional>

#include "asyncio.h"
#include "backend.h"

#ifndef DISABLE_URING
#include "uring.h"
#endif

#ifndef DISABLE_AIO
#include "aio.h"
#endif

class AsyncFileWriter
{
public:
    AsyncFileWriter(int fd, unsigned int n_entries, const std::string &backend);
    void write(size_t buffer, size_t n_bytes, unsigned long long offset, callback_t callback);
    void write_tensor(torch::Tensor tensor, unsigned long long offset, callback_t callback, std::optional<torch::Tensor> pinned);
    void synchronize();
    void register_h2d(unsigned int num_tensors);
    void sync_h2d();
    ~AsyncFileWriter();

private:
    int fd;
    AsyncIO *aio;
};