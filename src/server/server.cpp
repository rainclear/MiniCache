#include "server/server.hpp"
#include "protocol/command_factory.hpp"
#include "protocol/resp_parser.hpp"
#include "net/socket_utils.hpp"
#include <sys/epoll.h>

namespace minicache {

Server::Server(CacheStore& store, 
               AofEngine* aof, 
               int port, 
               std::shared_ptr<ObjectPool<net::NetworkBuffer>> buffer_pool,
               std::shared_ptr<ThreadPool> thread_pool)
    : store_(store), 
      aof_(aof), 
      port_(port), 
      buffer_pool_(std::move(buffer_pool)), 
      thread_pool_(std::move(thread_pool)),
      acceptor_(port) {
    if (!buffer_pool_) {
        buffer_pool_ = std::make_shared<ObjectPool<net::NetworkBuffer>>(16);
    }
}

Server::~Server() {
    stop();
}

void Server::start() {
    if (!acceptor_.listen()) return;

    acceptor_.set_new_connection_callback([this](int conn_fd) {
        on_new_connection(conn_fd);
    });

    reactor_.add_event(acceptor_.listen_fd(), EPOLLIN, [this]() {
        acceptor_.handle_accept();
    });

    server_thread_ = std::jthread([this](std::stop_token stop_tok) {
        reactor_.event_loop(stop_tok);
    });
}

void Server::stop() {
    if (server_thread_.joinable()) {
        server_thread_.request_stop();
        server_thread_.join();
    }
}

void Server::on_new_connection(int conn_fd) {
    auto conn = std::make_unique<net::Connection>(conn_fd, buffer_pool_);
    
    conn->set_message_callback([this](int fd, std::string_view raw) {
        on_message(fd, raw);
    });

    conn->set_close_callback([this](int fd) {
        on_close(fd);
    });

    net::Connection* conn_ptr = conn.get();
    connections_[conn_fd] = std::move(conn);

    reactor_.add_event(conn_fd, EPOLLIN | EPOLLET, [conn_ptr]() {
        conn_ptr->handle_read();
    });
}

void Server::on_message(int fd, std::string_view raw_data) {
    std::string parsed_cmd = RespParser::parse_array_to_cmd(raw_data);
    if (parsed_cmd.empty()) return;

    auto execute_cmd_task = [this, fd, parsed_cmd]() {
        auto it = connections_.find(fd);
        if (it == connections_.end()) return;

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

        it->second->send(resp_out);
    };

    if (thread_pool_) {
        thread_pool_->enqueue(execute_cmd_task);
    } else {
        execute_cmd_task();
    }
}

void Server::on_close(int fd) {
    reactor_.remove_event(fd);
    connections_.erase(fd);
}

} // namespace minicache