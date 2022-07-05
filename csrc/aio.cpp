#include <stdexcept>
#include <memory>
#include "aio.h"

AIOAsyncIO::AIOAsyncIO(int max_events)
{
    // printf("Initializing the io Context\n");
    io_setup(max_events, &(this->io_ctx)); /* 初始化ioctx*/
    this->timeout.tv_sec = 0;
    this->timeout.tv_nsec = 100000000;
}

void AIOAsyncIO::register_file(int fd) {}

AIOAsyncIO::~AIOAsyncIO()
{
    // printf("Closing AsyncIO context\n");
    synchronize();
    io_destroy(this->io_ctx);
}

void AIOAsyncIO::wait()
{
    auto *events = new io_event[this->max_nr]; /* pending I/O max support */
    int num_events;

    num_events = io_getevents(io_ctx, this->min_nr, this->max_nr, events, &(this->timeout)); /* 获得异步I/O event个数 */

    for (int i = 0; i < num_events; i++) /* 开始获取每一个event并且做相应处理 */
    {
        struct io_event event = events[i];
        auto *data = (IOData *)event.data;
        if (data->type == WRITE)
            this->n_write_events--;
        else if (data->type == READ)
            this->n_read_events--;
        else
            throw std::runtime_error("Unknown IO event type");
        if (data->callback != nullptr)
            data->callback();
        // printf("%d tasks to be done\n", this->n_write_events);
    }
}

void AIOAsyncIO::write(int fd, void *buffer, size_t n_bytes, unsigned long long offset, callback_t callback)
{
    struct iocb iocb
    {
    }; //建立一个异步I/O需求
    struct iocb *iocbs = &iocb;
    auto *data = new IOData(WRITE, callback);

    io_prep_pwrite(&iocb, fd, buffer, n_bytes, (long long)offset); // 初始化这个异步I/O需求 counter为偏移量

    data->type = WRITE;
    data->iov.iov_base = buffer;
    data->iov.iov_len = n_bytes;
    iocb.data = data;
    io_submit(this->io_ctx, 1, &iocbs); // 提交这个I/O不会堵塞

    this->n_write_events++;
}

void AIOAsyncIO::read(int fd, void *buffer, size_t n_bytes, unsigned long long offset, callback_t callback)
{
    struct iocb iocb
    {
    }; //建立一个异步I/O需求
    struct iocb *iocbs = &iocb;
    auto *data = new IOData(READ, callback);

    io_prep_pwrite(&iocb, fd, buffer, n_bytes, (long long)offset);

    data->type = READ;
    data->iov.iov_base = buffer;
    data->iov.iov_len = n_bytes;
    iocb.data = data;
    io_submit(this->io_ctx, 1, &iocbs); /* 提交这个I/O不会堵塞 */

    this->n_read_events++;
}

void AIOAsyncIO::sync_write_events()
{
    while (this->n_write_events > 0)
        wait();
}

void AIOAsyncIO::sync_read_events()
{
    while (this->n_read_events > 0)
        wait();
}

void AIOAsyncIO::synchronize()
{
    while (this->n_write_events > 0 || this->n_read_events > 0)
        wait();
}
