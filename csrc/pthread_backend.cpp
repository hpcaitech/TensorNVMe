#include "pthread_backend.h"
#include <fstream>

void write_file(const std::string &filename, const std::string &content)
{
    std::ofstream file(filename, std::ios::app);
    file << content << std::endl;
    file.close();
}

void PthreadAsyncIO::write(int fd, void *buffer, size_t n_bytes, unsigned long long offset, callback_t callback)
{
    auto fut = this->pool.submit_task(
        [this, fd, buffer, n_bytes, offset]
        {
            auto val = pwrite(fd, buffer, n_bytes, offset);
            if (this->is_debug)
            {
                auto cur_tasks = this->tasks_in_progress.fetch_sub(1);
                if (cur_tasks == 1)
                {
                    if (this->debug_log.empty())
                    {
                        std::cout << "All tasks are completed" << std::endl;
                    }
                    else
                    {
                        write_file(this->debug_log, "All tasks are completed");
                    }
                }
            }
            return val;
        });
    this->write_fut.push_back(std::make_tuple(std::move(fut), callback));
}

void PthreadAsyncIO::writev(int fd, const iovec *iov, unsigned int iovcnt, unsigned long long offset, callback_t callback)
{
    auto fut = this->pool.submit_task(
        [fd, iov, iovcnt, offset]
        {
            return pwritev(fd, iov, iovcnt, offset);
        });
    this->write_fut.push_back(std::make_tuple(std::move(fut), callback));
}

void PthreadAsyncIO::read(int fd, void *buffer, size_t n_bytes, unsigned long long offset, callback_t callback)
{
    auto fut = this->pool.submit_task(
        [fd, buffer, n_bytes, offset]
        {
            return pread(fd, buffer, n_bytes, offset);
        });
    this->read_fut.push_back(std::make_tuple(std::move(fut), callback));
}

void PthreadAsyncIO::readv(int fd, const iovec *iov, unsigned int iovcnt, unsigned long long offset, callback_t callback)
{
    auto fut = this->pool.submit_task(
        [fd, iov, iovcnt, offset]
        {
            return preadv(fd, iov, iovcnt, offset);
        });
    this->read_fut.push_back(std::make_tuple(std::move(fut), callback));
}

void PthreadAsyncIO::get_event(WaitType wt)
{
    if (wt == NOWAIT)
        return;
    this->sync_write_events();
    this->sync_read_events();
}

void PthreadAsyncIO::sync_write_events()
{
    while (this->write_fut.size() > 0)
    {
        auto front = std::move(this->write_fut.front());
        this->write_fut.pop_front();

        auto fut(std::move(std::get<0>(front)));
        fut.wait();

        auto callback = std::get<1>(front);
        if (callback != nullptr)
        {
            callback();
        }
    }
}

void PthreadAsyncIO::sync_read_events()
{
    while (this->read_fut.size() > 0)
    {
        auto front = std::move(this->read_fut.front());
        this->read_fut.pop_front();

        auto fut(std::move(std::get<0>(front)));
        fut.wait();

        auto callback = std::get<1>(front);
        if (callback != nullptr)
        {
            callback();
        }
    }
}

void PthreadAsyncIO::synchronize()
{
    this->get_event(WAIT);
}

void PthreadAsyncIO::register_file(int fd) {}

void PthreadAsyncIO::register_h2d(unsigned int num_tensors)
{
    this->h2d_in_progress.store(num_tensors); // register tensors to write for this run
}

void PthreadAsyncIO::sync_h2d()
{
    std::unique_lock<std::mutex> lock(this->mtx);
    this->cv.wait(lock, [this]
                  { return this->h2d_in_progress == 0; }); // block until all in-progress h2d are completed
}

void PthreadAsyncIO::write_tensor(int fd, torch::Tensor t, unsigned long long offset, callback_t callback, std::optional<torch::Tensor> pinned)
{
    auto stream = c10::cuda::getCurrentCUDAStream();
    if (!t.is_cuda())
    {
        this->h2d_in_progress.fetch_sub(1); // already moved to cpu
        if (this->h2d_in_progress.load() == 0)
        { // notify when all h2d are completed and safe to optimizer.step()
            std::lock_guard<std::mutex> lock(this->mtx);
            cv.notify_one();
        }
    }
    auto fut = this->pool.submit_task(
        [this, fd, t, offset, pinned, stream]
        {
            torch::Tensor cpu_tensor;
            if (t.is_cuda())
            {
                at::cuda::CUDAStreamGuard guard(stream); // https://pytorch.org/cppdocs/notes/tensor_cuda_stream.html
                if (pinned.has_value())
                {
                    pinned.value().copy_(t, /*non_blocking*/ false);
                    cpu_tensor = pinned.value();
                }
                else
                {
                    cpu_tensor = t.to(t.options().device(c10::DeviceType::CPU), /*non_blocking*/ false, /*copy*/ false); // modified from torch::Tensor::cpu()
                }
                this->h2d_in_progress.fetch_sub(1);
                if (this->h2d_in_progress.load() == 0)
                { // notify when all h2d are completed and safe to optimizer.step()
                    std::lock_guard<std::mutex> lock(this->mtx);
                    cv.notify_one();
                }
            }
            else
            {
                cpu_tensor = t;
            }
            void *buf = cpu_tensor.data_ptr();
            size_t n_bytes = cpu_tensor.numel() * cpu_tensor.element_size();
            auto val = pwrite(fd, buf, n_bytes, offset);
            if (this->is_debug)
            {
                auto cur_tasks = this->tasks_in_progress.fetch_sub(1);
                if (cur_tasks == 1)
                {
                    if (this->debug_log.empty())
                    {
                        std::cout << "All tasks are completed" << std::endl;
                    }
                    else
                    {
                        write_file(this->debug_log, "All tasks are completed");
                    }
                }
            }
            return val;
        });
    this->write_fut.push_back(std::make_tuple(std::move(fut), callback));
}

void PthreadAsyncIO::register_tasks(unsigned int num_tasks)
{
    if (this->is_debug)
    {
        this->tasks_in_progress.store(num_tasks);
    }
}