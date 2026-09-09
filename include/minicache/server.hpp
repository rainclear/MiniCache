#ifndef MINICACHE_SERVER_HPP
#define MINICACHE_SERVER_HPP

#include "minicache/cache_store.hpp"
#include "minicache/aof_engine.hpp"
#include <string>
#include <thread>
#include <stop_token>

namespace minicache {

/**
 * @brief Non-blocking Epoll-based TCP Server enabling Redis client compatibility.
 */
class Server {
public:
    Server(CacheStore& store, AofEngine* aof = nullptr, int port = 6379);
    ~Server();

    // Non-copyable, non-movable
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;
    Server(Server&&) = delete;
    Server& operator=(Server&&) = delete;

    /**
     * @brief Starts the background TCP server event loop.
     */
    void start();

    /**
     * @brief Signals the server event loop to stop and closes socket resources.
     */
    void stop();

private:
    void event_loop(std::stop_token stop_tok);
    static void set_nonblocking(int fd);

    CacheStore& store_;
    AofEngine* aof_;
    int port_;
    int listen_fd_{-1};
    int epoll_fd_{-1};
    std::jthread server_thread_;
};

} // namespace minicache

#endif // MINICACHE_SERVER_HPP