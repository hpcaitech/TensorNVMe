#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/uio.h>

#include "asyncio.h"
#include "threadpool.h"
#include "pthread_backend.h"


void AIOContext::worker(void *op_) {
    PthradIOData *op = reinterpret_cast<PthradIOData *>(op_);

    int fileno = op->fileno;
    off_t offset = op->offset;
    int buf_size = op->buf_size;
    void* buf = op->buf;
    const iovec* iov = op->iov;
    const callback_t cb = op->callback;
    std::atomic<unsigned int> *p_cnt = op->p_cnt;

    int result = -1;

    switch (op->type) {
        case WRITE:
            result = pwrite(fileno, buf, buf_size, offset);
            break;
        case WRITEV:
            result = pwritev(fileno, iov, buf_size, offset);
            break;
        case READ:
            result = pread(fileno, buf, buf_size, offset);
            break;
        case READV:
            result = preadv(fileno, iov, buf_size, offset);
            break;
        default:
            throw std::runtime_error("Unkown task");
    }

    if (cb != nullptr) {
        cb();
    }

    if (result < 0) {
        throw std::runtime_error("Error when executing tasks");
    }

    p_cnt->fetch_sub(1);
    delete op;
}

void AIOContext::submit(PthradIOData *op) {
    int result;
    do {
        result = threadpool_add(
            this->pool,
            AIOContext::worker,
            static_cast<void *>(op),
            0
        );
    } while (result == threadpool_queue_full);

    if (result < 0) {
        throw std::runtime_error("error when submitting job");
    }
}

int PthreadAsyncIO::getEnvValue(const char* varName, int defaultValue) {
    const char* envVar = std::getenv(varName);
    if (envVar != nullptr) {
        std::stringstream ss(envVar);
        int value;
        // Try converting to an integer
        if (ss >> value) {
            return value;
        } else {
            throw std::runtime_error("Failed to parse integer environ");
        }
    }
    return defaultValue;
}

void PthreadAsyncIO::write(int fd, void *buffer, size_t n_bytes, unsigned long long offset, callback_t callback) {
    this->n_write_events.fetch_add(1);
    PthradIOData *op = new PthradIOData(
        WRITE,
        fd,
        offset,
        n_bytes,
        buffer,
        nullptr,
        callback,
        &this->n_write_events
    );
    this->ctx.submit(op);
}

void PthreadAsyncIO::read(int fd, void *buffer, size_t n_bytes, unsigned long long offset, callback_t callback) {
    this->n_read_events.fetch_add(1);
    PthradIOData *op = new PthradIOData(
        READ,
        fd,
        offset,
        n_bytes,
        buffer,
        nullptr,
        callback,
        &this->n_read_events
    );
    this->ctx.submit(op);
}


void PthreadAsyncIO::writev(int fd, const iovec *iov, unsigned int iovcnt, unsigned long long offset, callback_t callback) {
    this->n_write_events.fetch_add(1);
    PthradIOData *op = new PthradIOData(
        WRITEV,
        fd,
        offset,
        static_cast<unsigned long long>(iovcnt),
        nullptr,
        iov,
        callback,
        &this->n_write_events
    );
    this->ctx.submit(op);
}

void PthreadAsyncIO::readv(int fd, const iovec *iov, unsigned int iovcnt, unsigned long long offset, callback_t callback) {
    this->n_read_events.fetch_add(1);
    PthradIOData *op = new PthradIOData(
        READV,
        fd,
        offset,
        static_cast<unsigned long long>(iovcnt),
        nullptr,
        iov,
        callback,
        &this->n_read_events
    );
    this->ctx.submit(op);
}

void PthreadAsyncIO::get_event(WaitType wt) {
    if (wt == NOWAIT) return;
    // busy waiting
    while (
        this->n_write_events.load() != 0
        || this->n_read_events.load() != 0
    ) {}
}

void PthreadAsyncIO::sync_write_events() {
    while (this->n_write_events.load() != 0) {}
}

void PthreadAsyncIO::sync_read_events() {
    while (this->n_read_events.load() != 0) {}
}

void PthreadAsyncIO::synchronize() {
    this->get_event(WAIT);
}

void PthreadAsyncIO::register_file(int fd) {}