#ifndef MINICACHE_NET_ACCEPTOR_HPP
#define MINICACHE_NET_ACCEPTOR_HPP

#include <functional>

namespace minicache::net {

class Acceptor {
public:
    using NewConnectionCallback = std::function<void(int conn_fd)>;

    explicit Acceptor(int port);
    ~Acceptor();

    Acceptor(const Acceptor&) = delete;
    Acceptor& operator=(const Acceptor&) = delete;

    bool listen();
    void set_new_connection_callback(NewConnectionCallback cb) {
        new_connection_cb_ = std::move(cb);
    }

    [[nodiscard]] int listen_fd() const noexcept { return listen_fd_; }
    void handle_accept();

private:
    int port_;
    int listen_fd_{-1};
    NewConnectionCallback new_connection_cb_;
};

} // namespace minicache::net

#endif // MINICACHE_NET_ACCEPTOR_HPP