#pragma once
#include "asyncio.h"
#ifndef DISABLE_URING
#include "uring.h"
#endif
#ifndef DISABLE_AIO
#include "aio.h"
#endif

class AsyncFileIO
{
public:
    AsyncFileIO(int fd, unsigned int n_entries);
    void write(void *buffer, size_t n_bytes, unsigned long long offset);
    void read(void *buffer, size_t n_bytes, unsigned long long offset);
    void synchronize();
    ~AsyncFileIO();

private:
    int fd;
    AsyncIO *aio;
};