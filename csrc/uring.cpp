#include <stdexcept>
#include <memory>
#include "uring.h"

UringAsyncIO::UringAsyncIO(unsigned int n_entries) : n_write_events(0), n_read_events(0), n_entries(n_entries)
{
    io_uring_queue_init(n_entries, &this->ring, 0);
}

void UringAsyncIO::register_file(int fd)
{
    io_uring_register_files(&ring, &fd, 1);
}

UringAsyncIO::~UringAsyncIO()
{
    synchronize();
    io_uring_queue_exit(&this->ring);
}

void UringAsyncIO::get_event(WaitType wt)
{
    io_uring_cqe *cqe;
    if (wt == WAIT){
        io_uring_wait_cqe(&this->ring, &cqe);
    }
    else{
        int ret = io_uring_peek_cqe(&this->ring, &cqe);
        if (ret != 0) return;
    }

    std::unique_ptr<IOData> data(static_cast<IOData *>(io_uring_cqe_get_data(cqe)));
    if (data->type == WRITE)
        this->n_write_events--;
    else if (data->type == READ)
        this->n_read_events--;
    else
        throw std::runtime_error("Unknown IO event type");
    io_uring_cqe_seen(&this->ring, cqe);
    if (data->callback != nullptr)
        data->callback();
}

void UringAsyncIO::write(int fd, void *buffer, size_t n_bytes, unsigned long long offset, callback_t callback)
{
    io_uring_sqe *sqe = io_uring_get_sqe(&this->ring);
    IOData *data = new IOData(WRITE, callback);
    io_uring_prep_write(sqe, fd, buffer, n_bytes, offset);
    io_uring_sqe_set_data(sqe, data);
    io_uring_submit(&this->ring);
    this->n_write_events++;
}

void UringAsyncIO::read(int fd, void *buffer, size_t n_bytes, unsigned long long offset, callback_t callback)
{
    io_uring_sqe *sqe = io_uring_get_sqe(&this->ring);
    IOData *data = new IOData(READ, callback);
    io_uring_prep_read(sqe, fd, buffer, n_bytes, offset);
    io_uring_sqe_set_data(sqe, data);
    io_uring_submit(&this->ring);
    this->n_read_events++;
}

void UringAsyncIO::sync_write_events()
{
    while (this->n_write_events > 0)
        get_event(WAIT);
}

void UringAsyncIO::sync_read_events()
{
    while (this->n_read_events > 0)
        get_event(WAIT);
}

void UringAsyncIO::synchronize()
{
    while (this->n_write_events > 0 || this->n_read_events > 0)
        get_event(WAIT);
}

void UringAsyncIO::writev(int fd, const iovec *iov, unsigned int iovcnt, unsigned long long offset, callback_t callback)
{
    io_uring_sqe *sqe = io_uring_get_sqe(&this->ring);
    IOData *data = new IOData(WRITE, callback, iov);
    io_uring_prep_writev(sqe, fd, iov, iovcnt, offset);
    io_uring_sqe_set_data(sqe, data);
    io_uring_submit(&this->ring);
    this->n_write_events++;
}

void UringAsyncIO::readv(int fd, const iovec *iov, unsigned int iovcnt, unsigned long long offset, callback_t callback)
{
    io_uring_sqe *sqe = io_uring_get_sqe(&this->ring);
    IOData *data = new IOData(READ, callback, iov);
    io_uring_prep_readv(sqe, fd, iov, iovcnt, offset);
    io_uring_sqe_set_data(sqe, data);
    io_uring_submit(&this->ring);
    this->n_read_events++;
}

void UringAsyncIO::write_tensor(int fd, torch::Tensor t, unsigned long long offset, callback_t callback, std::optional<torch::Tensor> pinned) {
    if (t.is_cuda()) {
        if (pinned.has_value()) {
            pinned.value().copy_(t);
            t = pinned.value();
        } else {
            t = t.to(torch::kCPU);
        }
    }
    void *buffer = t.data_ptr<float>();
    size_t n_bytes = t.numel() * t.element_size();
    this->write(fd, buffer, n_bytes, offset, callback);
}

void UringAsyncIO::register_h2d(unsigned int num_tensors) {}
void UringAsyncIO::sync_h2d() {}