#include "minicache/server.hpp"
#include "minicache/command_factory.hpp"
#include "minicache/resp_parser.hpp"

#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <cstring>

namespace minicache {

constexpr int kMaxEvents = 64;

Server::Server(CacheStore& store, AofEngine* aof, int port, std::shared_ptr<ObjectPool<NetworkBuffer>> buffer_pool)
    : store_(store), aof_(aof), port_(port), buffer_pool_(std::move(buffer_pool)) {
    if (!buffer_pool_) {
        // Instantiate a default pool with 16 pre-allocated network buffers
        buffer_pool_ = std::make_shared<ObjectPool<NetworkBuffer>>(16);
    }
}

Server::~Server() {
    stop();
}

void Server::set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void Server::start() {
    server_thread_ = std::jthread([this](std::stop_token stop_tok) {
        event_loop(stop_tok);
    });
}

void Server::stop() {
    if (server_thread_.joinable()) {
        server_thread_.request_stop();
        server_thread_.join();
    }
    if (listen_fd_ != -1) {
        close(listen_fd_);
        listen_fd_ = -1;
    }
    if (epoll_fd_ != -1) {
        close(epoll_fd_);
        epoll_fd_ = -1;
    }
}

void Server::event_loop(std::stop_token stop_tok) {
    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0) return;

    int opt = 1;
    setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    set_nonblocking(listen_fd_);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (bind(listen_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) return;
    if (listen(listen_fd_, SOMAXCONN) < 0) return;

    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ < 0) return;

    epoll_event ev{}, events[kMaxEvents];
    ev.events = EPOLLIN;
    ev.data.fd = listen_fd_;
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, listen_fd_, &ev);

    while (!stop_tok.stop_requested()) {
        int nfds = epoll_wait(epoll_fd_, events, kMaxEvents, 100);
        for (int i = 0; i < nfds; ++i) {
            int client_fd = events[i].data.fd;

            if (client_fd == listen_fd_) {
                sockaddr_in client_addr{};
                socklen_t client_len = sizeof(client_addr);
                int conn_fd = accept(listen_fd_, (struct sockaddr*)&client_addr, &client_len);
                if (conn_fd >= 0) {
                    set_nonblocking(conn_fd);
                    ev.events = EPOLLIN | EPOLLET;
                    ev.data.fd = conn_fd;
                    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, conn_fd, &ev);
                }
            } else {
                // Scope block: Acquire pooled buffer for zero-allocation I/O
                {
                    auto io_buf = buffer_pool_->acquire();
                    ssize_t bytes_read = read(client_fd, io_buf->data(), io_buf->capacity() - 1);

                    if (bytes_read <= 0) {
                        epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, client_fd, nullptr);
                        close(client_fd);
                    } else {
                        io_buf->set_size(static_cast<std::size_t>(bytes_read));
                        io_buf->data()[bytes_read] = '\0';

                        std::string_view raw_input(io_buf->data(), io_buf->size());
                        std::string parsed_cmd = RespParser::parse_array_to_cmd(raw_input);

                        if (!parsed_cmd.empty()) {
                            auto cmd = CommandFactory::parse(parsed_cmd);
                            std::string resp_out;

                            if (cmd) {
                                std::string res = cmd->execute(store_);
                                if (aof_ && (parsed_cmd.rfind("SET", 0) == 0 || parsed_cmd.rfind("DEL", 0) == 0)) {
                                    aof_->append(parsed_cmd);
                                }

                                if (res == "OK") resp_out = RespParser::serialize_simple_string("OK");
                                else if (res == "(nil)") resp_out = RespParser::serialize_null();
                                else if (res.rfind("(integer)", 0) == 0) resp_out = RespParser::serialize_simple_string(res);
                                else resp_out = RespParser::serialize_bulk_string(res);
                            } else {
                                resp_out = RespParser::serialize_error("unknown command");
                            }

                            write(client_fd, resp_out.data(), resp_out.size());
                        }
                    }
                } // io_buf leaves scope -> reset() called & returned to ObjectPool automatically!
            }
        }
    }
}

} // namespace minicache