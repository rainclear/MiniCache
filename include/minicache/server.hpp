#ifndef MINICACHE_SERVER_HPP
#define MINICACHE_SERVER_HPP

#include "minicache/cache_store.hpp"
#include "minicache/aof_engine.hpp"
#include "minicache/object_pool.hpp"
#include <string>
#include <thread>
#include <stop_token>
#include <memory>
#include <cstring>
#include <algorithm>

namespace minicache {

/**
 * @brief Reusable network buffer satisfying C++20 Resettable concept.
 */
class NetworkBuffer {
public:
    static constexpr std::size_t kCapacity = 4096;

    NetworkBuffer() {
        data_.fill(0);
    }

    void reset() {
        std::fill(data_.begin(), data_.begin() + size_, 0);
        size_ = 0;
    }

    [[nodiscard]] char* data() noexcept { return data_.data(); }
    [[nodiscard]] const char* data() const noexcept { return data_.data(); }
    [[nodiscard]] std::size_t capacity() const noexcept { return kCapacity; }
    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    void set_size(std::size_t s) noexcept { size_ = std::min(s, kCapacity); }

private:
    std::array<char, kCapacity> data_;
    std::size_t size_{0};
};

/**
 * @brief Non-blocking Epoll-based TCP Server integrated with ObjectPool.
 */
class Server {
public:
    Server(CacheStore& store, 
           AofEngine* aof = nullptr, 
           int port = 6379,
           std::shared_ptr<ObjectPool<NetworkBuffer>> buffer_pool = nullptr);
    ~Server();

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;
    Server(Server&&) = delete;
    Server& operator=(Server&&) = delete;

    void start();
    void stop();

private:
    void event_loop(std::stop_token stop_tok);
    static void set_nonblocking(int fd);

    CacheStore& store_;
    AofEngine* aof_;
    int port_;
    std::shared_ptr<ObjectPool<NetworkBuffer>> buffer_pool_;
    int listen_fd_{-1};
    int epoll_fd_{-1};
    std::jthread server_thread_;
};

} // namespace minicache

#endif // MINICACHE_SERVER_HPP