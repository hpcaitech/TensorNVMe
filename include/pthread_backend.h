#pragma once

#include <list>
#include <stdexcept>
#include <sys/io.h>
#include <cstdlib>
#include <sstream>
#include "asyncio.h"
#include "threadpool.h"


static const unsigned int PTHREAD_POOL_SIZE_DEFAULT = 8;
static const char* PTHREAD_POOL_SIZE_ENVIRON_NAME = "PTHREAD_POOL_SIZE";

class PthradIOData;

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
            threadpool_destroy(pool, 0);  // wait all threads
        }
    }

    void submit(PthradIOData *op);

    static void worker(void *op);

private:
    threadpool_t *pool;
    unsigned int max_requests;
    unsigned int pool_size;

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
        const callback_t callback_
    )
        : IOData(type_, callback_, iov_)
        , fileno(fileno_)
        , offset(offset_)
        , result(-1)
        , error(0)
        , in_progress(false)
        , buf_size(buf_size_)
        , buf(buf_)
    {}

    ~PthradIOData() = default;  // release iov_ by calling parent destructor
private:
    const int fileno;
    const unsigned long long offset;
    int result;
    int error;
    bool in_progress;
    const unsigned long long buf_size;
    void* buf;
};


class PthreadAsyncIO : public AsyncIO
{
private:
    AIOContext ctx;

    static int getEnvValue(const char* varName, int defaultValue) {
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

public:
    PthreadAsyncIO(unsigned int n_entries)
        : ctx(
            n_entries,
            getEnvValue(
                PTHREAD_POOL_SIZE_ENVIRON_NAME,
                PTHREAD_POOL_SIZE_DEFAULT
            )
        ) {}

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