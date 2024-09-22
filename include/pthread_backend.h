#pragma once

#include <list>
#include <stdexcept>
#include <sys/io.h>
#include <cstdlib>
#include <sstream>
#include <atomic>
#include "asyncio.h"
#include "threadpool.h"


static const unsigned int PTHREAD_POOL_SIZE_DEFAULT = 8;
static const char* PTHREAD_POOL_SIZE_ENVIRON_NAME = "PTHREAD_POOL_SIZE";

class PthradIOData;

// thread pool wrapper
class AIOContext {
public:
    AIOContext(
        unsigned int max_requests,
        unsigned int pool_size
    ) 
        : pool(threadpool_create(max_requests, pool_size, 0))
    {
        if (this->pool == nullptr) {
            throw std::runtime_error("failed to allocate thread pool");
        }
    }

    ~AIOContext() {
        if (this->pool != nullptr) {
            threadpool_t* pool = this->pool;
            this->pool = nullptr;
            threadpool_destroy(pool, 0);  // wait all threads
        }
    }

    void submit(PthradIOData *op);

    static void worker(void *op);

private:
    threadpool_t *pool;
};

// data class
class PthradIOData: IOData {
    friend class AIOContext;

public:
    PthradIOData(
        const IOType type_,
        const int fileno_,
        const unsigned long long offset_,
        const unsigned long long buf_size_,
        void* buf_,
        const iovec *iov_,
        const callback_t callback_,
        std::atomic<unsigned int> *p_cnt_
    )
        : IOData(type_, callback_, iov_)
        , fileno(fileno_)
        , offset(offset_)
        , buf_size(buf_size_)
        , buf(buf_)
        , p_cnt(p_cnt_)
    {}

    ~PthradIOData() = default;  // release iov_ by calling parent destructor

private:
    const int fileno;
    const unsigned long long offset;
    const unsigned long long buf_size;
    void* buf;
    std::atomic<unsigned int> *p_cnt;
};


class PthreadAsyncIO : public AsyncIO
{
private:
    AIOContext ctx;
    std::atomic<unsigned int> n_write_events;  // atomic is needed here since this value could be updated by multiple threads
    std::atomic<unsigned int> n_read_events;   // atomic is needed here since this value could be updated by multiple threads

    static int getEnvValue(const char* varName, int defaultValue);
public:
    PthreadAsyncIO(unsigned int n_entries)
        : ctx(
            n_entries,
            getEnvValue(
                PTHREAD_POOL_SIZE_ENVIRON_NAME,
                PTHREAD_POOL_SIZE_DEFAULT
            )
        )
        , n_write_events(0)
        , n_read_events(0) {}

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