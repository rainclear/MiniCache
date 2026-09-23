#ifndef MINICACHE_SERVER_HPP
#define MINICACHE_SERVER_HPP

#include "storage/cache_store.hpp"
#include "aof/aof_engine.hpp"
#include "common/object_pool.hpp"
#include "common/thread_pool.hpp"
#include "net/reactor.hpp"
#include "net/acceptor.hpp"
#include "net/connection.hpp"

#include <memory>
#include <unordered_map>
#include <thread>
#include <stop_token>

namespace minicache {

class Server {
public:
    Server(CacheStore& store, 
           AofEngine* aof = nullptr, 
           int port = 6379,
           std::shared_ptr<ObjectPool<net::NetworkBuffer>> buffer_pool = nullptr,
           std::shared_ptr<ThreadPool> thread_pool = nullptr);
    ~Server();

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    void start();
    void stop();

private:
    void on_new_connection(int conn_fd);
    void on_message(int fd, std::string_view raw_data);
    void on_close(int fd);

    CacheStore& store_;
    AofEngine* aof_;
    int port_;
    
    std::shared_ptr<ObjectPool<net::NetworkBuffer>> buffer_pool_;
    std::shared_ptr<ThreadPool> thread_pool_;

    net::Reactor reactor_;
    net::Acceptor acceptor_;
    
    std::unordered_map<int, std::unique_ptr<net::Connection>> connections_;
    std::jthread server_thread_;
};

} // namespace minicache

#endif // MINICACHE_SERVER_HPP