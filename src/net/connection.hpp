#ifndef MINICACHE_NET_CONNECTION_HPP
#define MINICACHE_NET_CONNECTION_HPP

#include "net/buffer.hpp"
#include "common/object_pool.hpp"
#include <memory>
#include <string_view>
#include <functional>

namespace minicache::net {

class Connection {
public:
    using MessageCallback = std::function<void(int fd, std::string_view raw_data)>;
    using CloseCallback = std::function<void(int fd)>;

    Connection(int fd, std::shared_ptr<ObjectPool<NetworkBuffer>> buffer_pool);
    ~Connection();

    int fd() const noexcept { return fd_; }
    void set_message_callback(MessageCallback cb) { message_cb_ = std::move(cb); }
    void set_close_callback(CloseCallback cb) { close_cb_ = std::move(cb); }

    void handle_read();
    void send(std::string_view message);

private:
    int fd_{-1};
    std::shared_ptr<ObjectPool<NetworkBuffer>> buffer_pool_;
    MessageCallback message_cb_;
    CloseCallback close_cb_;
};

} // namespace minicache::net

#endif // MINICACHE_NET_CONNECTION_HPP