#pragma once

#include <stdexcept>
#include <sys/io.h>
#include "asyncio.h"
#include "threadpool.h"


enum thaio_op_code_t {
    THAIO_READ,
    THAIO_READV,
    THAIO_WRITE,
    THAIO_WRITEV,
    THAIO_FSYNC,
    THAIO_FDSYNC,
    THAIO_NOOP,
};

static const unsigned int CTX_POOL_SIZE_DEFAULT = 8;
static const unsigned int CTX_MAX_REQUESTS_DEFAULT = 512;

class AIOOperation;

class AIOContext {
public:
    AIOContext(
        unsigned int max_requests_,
        unsigned int pool_size_
    ) 
        : pool(nullptr)
        , max_requests(max_requests_)
        , pool_size(pool_size_)
    {
        this->pool = threadpool_create(this->pool_size, this->max_requests, 0);
        if (this->pool == nullptr) {
            throw std::runtime_error("failed to allocate thread pool");
        }
    }

    ~AIOContext() {
        if (this->pool != nullptr) {
            threadpool_t* pool = this->pool;
            this->pool = nullptr;
            threadpool_destroy(pool, 0);
        }
    }


    void submit(AIOOperation &op);

    static void worker(AIOOperation &op);

private:
    threadpool_t *pool;
    unsigned int max_requests;
    unsigned int pool_size;

};

// data class
class AIOOperation {
    friend class AIOContext;

public:
    AIOOperation(
        const thaio_op_code_t opcode_,
        const int fileno_,
        const unsigned long long offset_,
        const unsigned long long buf_size_,
        void* buf_,
        const iovec *iov_,
        const callback_t callback_
    )
        : opcode(opcode_)
        , fileno(fileno_)
        , offset(offset_)
        , result(-1)
        , error(0)
        , in_progress(false)
        , buf_size(buf_size_)
        , buf(buf_)
        , iov(iov_)
        , callback(callback_)
    {}

    ~AIOOperation() = default;
private:
    const thaio_op_code_t opcode;
    const int fileno;
    const unsigned long long offset;
    int result;
    int error;
    bool in_progress;
    const unsigned long long buf_size;
    void* buf;
    const iovec *iov;
    const callback_t callback;
};


class PthreadAsyncIO : public AsyncIO
{
private:
    AIOContext ctx;
    

public:
    PthreadAsyncIO(
        unsigned int max_requests = CTX_MAX_REQUESTS_DEFAULT,
        unsigned int pool_size = CTX_POOL_SIZE_DEFAULT
    ): ctx(max_requests, pool_size) {}

    ~PthreadAsyncIO() = default;

    void write(int fd, void *buffer, size_t n_bytes, unsigned long long offset, callback_t callback);
    void read(int fd, void *buffer, size_t n_bytes, unsigned long long offset, callback_t callback);
    void writev(int fd, const iovec *iov, unsigned int iovcnt, unsigned long long offset, callback_t callback);
    void readv(int fd, const iovec *iov, unsigned int iovcnt, unsigned long long offset, callback_t callback);

    void get_event(WaitType wt);
    void sync_write_events();
    void sync_read_events();
    void synchronize();
    
    void register_file(int fd);
};