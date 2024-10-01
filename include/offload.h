#pragma once

#include "asyncio.h"
#include <ATen/ATen.h>

#include "space_mgr.h"
#ifndef DISABLE_URING
#include "uring.h"
#endif
#ifndef DISABLE_AIO
#include "aio.h"
#endif

#include <cstdlib>
class Offloader
{
public:
    Offloader(const std::string &filename, unsigned int n_entries, const std::string &backend = "uring");
    SpaceInfo prepare_write(const at::Tensor &tensor, const std::string &key);
    SpaceInfo prepare_read(const at::Tensor &tensor, const std::string &key);
    void async_write(const at::Tensor &tensor, const std::string &key, callback_t callback = nullptr);
    void async_read(const at::Tensor &tensor, const std::string &key, callback_t callback = nullptr);
    void sync_write(const at::Tensor &tensor, const std::string &key);
    void sync_read(const at::Tensor &tensor, const std::string &key);
    void sync_write_events();
    void sync_read_events();
    void synchronize();
    ~Offloader();
    SpaceInfo prepare_writev(const std::vector<at::Tensor> &tensors, const std::string &key);
    SpaceInfo prepare_readv(const std::vector<at::Tensor> &tensors, const std::string &key);
    void async_writev(const std::vector<at::Tensor> &tensors, const std::string &key, callback_t callback = nullptr);
    void async_readv(const std::vector<at::Tensor> &tensors, const std::string &key, callback_t callback = nullptr);
    void sync_writev(const std::vector<at::Tensor> &tensors, const std::string &key);
    void sync_readv(const std::vector<at::Tensor> &tensors, const std::string &key);
    static std::string get_default_backend();
private:
    const std::string filename;
    int fd;
    AsyncIO *aio;
    SpaceManager space_mgr;
    std::unordered_map<std::string, SpaceInfo> tensors_info;

    void release(ull offset, ull bytes, callback_t callback = nullptr);
};