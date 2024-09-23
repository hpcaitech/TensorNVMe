#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/uio.h>

#include "asyncio.h"
#include "pthread_backend.h"

#include <iostream>
void PthreadAsyncIO::write(int fd, void *buffer, size_t n_bytes, unsigned long long offset, callback_t callback) {
    auto fut = this->pool.submit_task(
        [fd, buffer, n_bytes, offset, callback] {
            int result = pwrite(fd, buffer, n_bytes, offset);
            if (callback != nullptr) {
                callback();
            }
            return result;
        }
    );
    this->write_fut.push_back(std::move(fut));
}

void PthreadAsyncIO::read(int fd, void *buffer, size_t n_bytes, unsigned long long offset, callback_t callback) {
    auto fut = this->pool.submit_task(
        [fd, buffer, n_bytes, offset, callback] {
            int result = pread(fd, buffer, n_bytes, offset);
            if (callback != nullptr) {
                callback();
            }
            return result;
        }
    );
    this->read_fut.push_back(std::move(fut));
}

void PthreadAsyncIO::readv(int fd, const iovec *iov, unsigned int iovcnt, unsigned long long offset, callback_t callback) {
        auto fut = this->pool.submit_task(
        [fd, iov, iovcnt, offset, callback] {
            int result = preadv(fd, iov, iovcnt, offset);
            if (callback != nullptr) {
                callback();
            }
            return result;
        }
    );
    this->read_fut.push_back(std::move(fut));
}

void PthreadAsyncIO::writev(int fd, const iovec *iov, unsigned int iovcnt, unsigned long long offset, callback_t callback) {
    auto fut = this->pool.submit_task(
        [fd, iov, iovcnt, offset, callback] {
            int result = pwritev(fd, iov, iovcnt, offset);
            if (callback != nullptr) {
                callback();
            }
            return result;
        }
    );
    this->write_fut.push_back(std::move(fut));
}

void PthreadAsyncIO::get_event(WaitType wt) {
    if (wt == NOWAIT) return;
    this->sync_write_events();
    this->sync_read_events();

}

void PthreadAsyncIO::sync_write_events() {
    while (this->write_fut.size() > 0) {
        std::cout << "hi:" << this->write_fut.size() << std::endl;
        auto front = std::move(this->write_fut.front());
        this->write_fut.pop_front();
        front.wait();
    }
}

void PthreadAsyncIO::sync_read_events() {
    while (this->read_fut.size() > 0) {
        std::cout << "ho:" << this->read_fut.size() << std::endl;
        auto front = std::move(this->read_fut.front());
        this->read_fut.pop_front();
        front.wait();
    }
}

void PthreadAsyncIO::synchronize() {
    this->get_event(WAIT);
}

void PthreadAsyncIO::register_file(int fd) {}