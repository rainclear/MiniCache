#ifndef MINICACHE_NET_REACTOR_HPP
#define MINICACHE_NET_REACTOR_HPP

#include <functional>
#include <unordered_map>
#include <stop_token>
#include <sys/epoll.h>

namespace minicache::net {

class Reactor {
public:
    using EventCallback = std::function<void()>;

    Reactor();
    ~Reactor();

    bool add_event(int fd, uint32_t events, EventCallback cb);
    bool remove_event(int fd);
    void event_loop(std::stop_token stop_tok);

private:
    static constexpr int kMaxEvents = 64;
    int epoll_fd_{-1};
    std::unordered_map<int, EventCallback> callbacks_;
};

} // namespace minicache::net

#endif // MINICACHE_NET_REACTOR_HPP