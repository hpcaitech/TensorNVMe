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

        std::future<ssize_t> fut(std::move(std::get<0>(front)));
        fut.wait();

        callback_t callback = std::get<1>(front);
        if (callback != nullptr) {
            callback();
        }
    }
}

void PthreadAsyncIO::sync_read_events() {
    while (this->read_fut.size() > 0) {
        auto front = std::move(this->read_fut.front());
        this->read_fut.pop_front();

        std::future<ssize_t> fut(std::move(std::get<0>(front)));
        fut.wait();

        callback_t callback = std::get<1>(front);
        if (callback != nullptr) {
            callback();
        }
    }
}

void PthreadAsyncIO::synchronize() {
    this->get_event(WAIT);
}

void PthreadAsyncIO::register_file(int fd) {}