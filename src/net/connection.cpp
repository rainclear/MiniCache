#include "net/connection.hpp"
#include <unistd.h>

namespace minicache::net {

Connection::Connection(int fd, std::shared_ptr<ObjectPool<NetworkBuffer>> buffer_pool)
    : fd_(fd), buffer_pool_(std::move(buffer_pool)) {}

Connection::~Connection() {
    if (fd_ != -1) {
        ::close(fd_);
    }
}

void Connection::handle_read() {
    auto io_buf = buffer_pool_->acquire();
    ssize_t bytes_read = ::read(fd_, io_buf->data(), io_buf->capacity() - 1);

    if (bytes_read <= 0) {
        if (close_cb_) close_cb_(fd_);
        return;
    }

    io_buf->set_size(static_cast<std::size_t>(bytes_read));
    io_buf->data()[bytes_read] = '\0';

    if (message_cb_) {
        message_cb_(fd_, std::string_view(io_buf->data(), io_buf->size()));
    }
}

void Connection::send(std::string_view message) {
    if (fd_ != -1) {
        ::write(fd_, message.data(), message.size());
    }
}

} // namespace minicache::net