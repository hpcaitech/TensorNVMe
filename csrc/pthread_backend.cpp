#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/uio.h>

#include "asyncio.h"
#include "threadpool.h"
#include "pthread_backend.h"


void AIOContext::worker(AIOOperation &op) {

    if (op.opcode == THAIO_NOOP) {
        return;
    }

    int fileno = op.fileno;
    off_t offset = op.offset;
    int buf_size = op.buf_size;
    void* buf = op.buf;
    const iovec* iov = op.iov;

    int result;

    switch (op.opcode) {
        case THAIO_WRITE:
            result = pwrite(fileno, buf, buf_size, offset);
            break;
        case THAIO_WRITEV:
            result = pwritev(fileno, iov, buf_size, offset);
            break;
        case THAIO_FSYNC:
            result = fsync(fileno);
            break;
        case THAIO_FDSYNC:
            result = fdatasync(fileno);
            break;
        case THAIO_READ:
            result = pread(fileno, buf, buf_size, offset);
            break;
        case THAIO_READV:
            result = preadv(fileno, iov, buf_size, offset);
            break;
    }

    op.result = result;

    if (result < 0) op.error = errno;

}

void AIOContext::submit(AIOOperation &op) {
    op.in_progress = true;
    int result = threadpool_add(
        this->pool,
        reinterpret_cast<void (*)(void *)>(AIOContext::worker),
        nullptr, // reinterpret_cast<void *>(&op), // TODO @botbw OP RAII?
        0
    );
    if (result < 0) {
        throw std::runtime_error("error when submitting job");
    }
}

void PthreadAsyncIO::write(int fd, void *buffer, size_t n_bytes, unsigned long long offset, callback_t callback) {
    AIOOperation op(
        THAIO_WRITE,
        fd,
        offset,
        n_bytes,
        buffer,
        nullptr
    );
    this->ctx.submit(op);
}

void PthreadAsyncIO::read(int fd, void *buffer, size_t n_bytes, unsigned long long offset, callback_t callback) {
    AIOOperation op(
        THAIO_READ,
        fd,
        offset,
        n_bytes,
        buffer,
        nullptr
    );
    this->ctx.submit(op);
}


void PthreadAsyncIO::writev(int fd, const iovec *iov, unsigned int iovcnt, unsigned long long offset, callback_t callback) {
        AIOOperation op(
        THAIO_WRITEV,
        fd,
        offset,
        static_cast<unsigned long long>(iovcnt),
        nullptr,
        iov
    );
    this->ctx.submit(op);
}

void PthreadAsyncIO::readv(int fd, const iovec *iov, unsigned int iovcnt, unsigned long long offset, callback_t callback) {
        AIOOperation op(
        THAIO_READV,
        fd,
        offset,
        static_cast<unsigned long long>(iovcnt),
        nullptr,
        iov
    );
    this->ctx.submit(op);
}

void PthreadAsyncIO::get_event(WaitType wt) {
    throw std::runtime_error("not implemented");
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