#include "pthread_backend.h"

void PthreadAsyncIO::write(int fd, void *buffer, size_t n_bytes, unsigned long long offset, callback_t callback) {
    auto fut = this->pool.submit_task(
        [fd, buffer, n_bytes, offset] {
            return pwrite(fd, buffer, n_bytes, offset);
        }
    );
    this->write_fut.push_back(std::make_tuple(std::move(fut), callback));
}

void PthreadAsyncIO::writev(int fd, const iovec *iov, unsigned int iovcnt, unsigned long long offset, callback_t callback) {
    auto fut = this->pool.submit_task(
        [fd, iov, iovcnt, offset] {
            return pwritev(fd, iov, iovcnt, offset);
        }
    );
    this->write_fut.push_back(std::make_tuple(std::move(fut), callback));
}

void PthreadAsyncIO::read(int fd, void *buffer, size_t n_bytes, unsigned long long offset, callback_t callback) {
    auto fut = this->pool.submit_task(
        [fd, buffer, n_bytes, offset] {
            return pread(fd, buffer, n_bytes, offset);
        }
    );
    this->read_fut.push_back(std::make_tuple(std::move(fut), callback));
}

void PthreadAsyncIO::readv(int fd, const iovec *iov, unsigned int iovcnt, unsigned long long offset, callback_t callback) {
        auto fut = this->pool.submit_task(
        [fd, iov, iovcnt, offset] {
            return preadv(fd, iov, iovcnt, offset);
        }
    );
    this->read_fut.push_back(std::make_tuple(std::move(fut), callback));
}

void PthreadAsyncIO::get_event(WaitType wt) {
    if (wt == NOWAIT) return;
    this->sync_write_events();
    this->sync_read_events();
}

void PthreadAsyncIO::sync_write_events() {
    while (this->write_fut.size() > 0) {
        auto front = std::move(this->write_fut.front());
        this->write_fut.pop_front();

        auto fut(std::move(std::get<0>(front)));
        fut.wait();

        auto callback = std::get<1>(front);
        if (callback != nullptr) {
            callback();
        }
    }
}

void PthreadAsyncIO::sync_read_events() {
    while (this->read_fut.size() > 0) {
        auto front = std::move(this->read_fut.front());
        this->read_fut.pop_front();

        auto fut(std::move(std::get<0>(front)));
        fut.wait();

        auto callback = std::get<1>(front);
        if (callback != nullptr) {
            callback();
        }
    }
}

void PthreadAsyncIO::synchronize() {
    this->get_event(WAIT);
}

void PthreadAsyncIO::register_file(int fd) {}

void PthreadAsyncIO::write_tensor(int fd, torch::Tensor t, unsigned long long offset, callback_t callback, std::optional<torch::Tensor> pinned) {
    auto stream = c10::cuda::getCurrentCUDAStream();
    at::cuda::CUDAStreamGuard guard(stream);  // https://pytorch.org/cppdocs/notes/tensor_cuda_stream.html
    auto event_ptr = std::make_shared<c10::Event>(torch::kCUDA);  // make a shared ptr here since event is not copyable
    if (t.is_cuda()) {
        if (pinned.has_value()) {
            pinned.value().copy_(t, /*non_blocking*/ true);
            t = pinned.value();
        } else {
            t = t.to(t.options().device(c10::DeviceType::CPU), /*non_blocking*/ true, /*copy*/ false);  // modified from torch::Tensor::cpu()
        }
    }
    event_ptr->record(stream);
    auto fut = this->pool.submit_task(
        [fd, t, offset, pinned, event_ptr] {
            event_ptr->synchronize(); // sync with comm stream
            void *buf = t.data_ptr();
            size_t n_bytes = t.numel() * t.element_size();
            return pwrite(fd, buf, n_bytes, offset);
        }
    );
    this->write_fut.push_back(std::make_tuple(std::move(fut), callback));
}