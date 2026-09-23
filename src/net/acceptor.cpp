#include "net/acceptor.hpp"
#include "net/socket_utils.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

namespace minicache::net {

Acceptor::Acceptor(int port) : port_(port) {}

Acceptor::~Acceptor() {
    if (listen_fd_ != -1) {
        ::close(listen_fd_);
    }
}

bool Acceptor::listen() {
    listen_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0) return false;

    int opt = 1;
    ::setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    set_nonblocking(listen_fd_);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (::bind(listen_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) return false;
    if (::listen(listen_fd_, SOMAXCONN) < 0) return false;

    return true;
}

void Acceptor::handle_accept() {
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    int conn_fd = ::accept(listen_fd_, (struct sockaddr*)&client_addr, &client_len);
    if (conn_fd >= 0) {
        set_nonblocking(conn_fd);
        if (new_connection_cb_) {
            new_connection_cb_(conn_fd);
        } else {
            ::close(conn_fd);
        }
    }
}

} // namespace minicache::net