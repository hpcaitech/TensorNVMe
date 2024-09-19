#pragma once
#include "asyncio.h"
#ifndef DISABLE_URING
#include "uring.h"
#endif
#ifndef DISABLE_AIO
#include "aio.h"
#endif

class AsyncFileWriter
{
public:
    AsyncFileWriter(int fd, unsigned int n_entries);
    void write(size_t buffer, size_t n_bytes, unsigned long long offset);
    void synchronize();
    ~AsyncFileWriter();

private:
    int fd;
    AsyncIO *aio;
};