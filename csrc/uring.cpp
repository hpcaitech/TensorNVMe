#include <stdexcept>
#include <memory>
#include "uring.h"

UringAsyncIO::UringAsyncIO(unsigned int n_entries) : n_write_events(0), n_read_events(0), n_entries(n_entries)
{
    io_uring_queue_init(n_entries, &this->ring, 0);
}

UringAsyncIO::~UringAsyncIO()
{
    synchronize();
    io_uring_queue_exit(&this->ring);
}

void UringAsyncIO::wait()
{
    io_uring_cqe *cqe;
    io_uring_wait_cqe(&this->ring, &cqe);
    std::unique_ptr<IOData> data(static_cast<IOData *>(io_uring_cqe_get_data(cqe)));
    if (data->type == WRITE)
        this->n_write_events--;
    else if (data->type == READ)
        this->n_read_events--;
    else
        throw std::runtime_error("Unknown IO event type");
    if (data->callback != nullptr)
        data->callback();
    io_uring_cqe_seen(&this->ring, cqe);
}

void UringAsyncIO::write(int fd, void *buffer, size_t n_bytes, unsigned long long offset, callback_t callback)
{
    io_uring_sqe *sqe = io_uring_get_sqe(&this->ring);
    IOData *data = new IOData(WRITE, callback);
    data->iov.iov_base = buffer;
    data->iov.iov_len = n_bytes;
    io_uring_prep_writev(sqe, fd, &data->iov, 1, offset);
    io_uring_sqe_set_data(sqe, data);
    io_uring_submit(&this->ring);
    this->n_write_events++;
}

void UringAsyncIO::read(int fd, void *buffer, size_t n_bytes, unsigned long long offset, callback_t callback)
{
    io_uring_sqe *sqe = io_uring_get_sqe(&this->ring);
    IOData *data = new IOData(READ, callback);
    data->iov.iov_base = buffer;
    data->iov.iov_len = n_bytes;
    io_uring_prep_readv(sqe, fd, &data->iov, 1, offset);
    io_uring_sqe_set_data(sqe, data);
    io_uring_submit(&this->ring);
    this->n_read_events++;
}

void UringAsyncIO::sync_write_events()
{
    while (this->n_write_events > 0)
        wait();
}

void UringAsyncIO::sync_read_events()
{
    while (this->n_read_events > 0)
        wait();
}

void UringAsyncIO::synchronize()
{
    while (this->n_write_events > 0 || this->n_read_events > 0)
        wait();
}
