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

    int result;

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
    }

    op->result = result;

    if (cb != nullptr) {
        cb();
    }

    if (result < 0) op->error = errno;

}

void AIOContext::submit(PthradIOData *op) {
    op->in_progress = true;
    int result = threadpool_add(
        this->pool,
        AIOContext::worker,
        static_cast<void *>(op),
        0
    );
    if (result < 0) {
        throw std::runtime_error("error when submitting job");
    }
}

void PthreadAsyncIO::write(int fd, void *buffer, size_t n_bytes, unsigned long long offset, callback_t callback) {
    PthradIOData *op = new PthradIOData(
        WRITE,
        fd,
        offset,
        n_bytes,
        buffer,
        nullptr,
        callback
    );
    this->ctx.submit(op);
}

void PthreadAsyncIO::read(int fd, void *buffer, size_t n_bytes, unsigned long long offset, callback_t callback) {
    PthradIOData *op = new PthradIOData(
        READ,
        fd,
        offset,
        n_bytes,
        buffer,
        nullptr,
        callback
    );
    this->ctx.submit(op);
}


void PthreadAsyncIO::writev(int fd, const iovec *iov, unsigned int iovcnt, unsigned long long offset, callback_t callback) {
        PthradIOData *op = new PthradIOData(
        WRITEV,
        fd,
        offset,
        static_cast<unsigned long long>(iovcnt),
        nullptr,
        iov,
        callback
    );
    this->ctx.submit(op);
}

void PthreadAsyncIO::readv(int fd, const iovec *iov, unsigned int iovcnt, unsigned long long offset, callback_t callback) {
        PthradIOData *op = new PthradIOData(
        READV,
        fd,
        offset,
        static_cast<unsigned long long>(iovcnt),
        nullptr,
        iov,
        callback
    );
    this->ctx.submit(op);
}

void PthreadAsyncIO::get_event(WaitType wt) {
    throw std::runtime_error("not implemented");
    // TODO @botbw: release PthreadIOData here
}

void PthreadAsyncIO::sync_write_events() {
    throw std::runtime_error("not implemented");
}

void PthreadAsyncIO::sync_read_events() {
    throw std::runtime_error("not implemented");
}

void PthreadAsyncIO::synchronize() {
    throw std::runtime_error("not implemented");
}

void PthreadAsyncIO::register_file(int fd) {}