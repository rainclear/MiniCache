#include "net/reactor.hpp"
#include <unistd.h>

namespace minicache::net {

Reactor::Reactor() {
    epoll_fd_ = epoll_create1(0);
}

Reactor::~Reactor() {
    if (epoll_fd_ != -1) {
        ::close(epoll_fd_);
    }
}

bool Reactor::add_event(int fd, uint32_t events, EventCallback cb) {
    if (epoll_fd_ < 0) return false;
    
    epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;

    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev) < 0) return false;
    callbacks_[fd] = std::move(cb);
    return true;
}

bool Reactor::remove_event(int fd) {
    if (epoll_fd_ < 0) return false;
    epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
    callbacks_.erase(fd);
    return true;
}

void Reactor::event_loop(std::stop_token stop_tok) {
    epoll_event events[kMaxEvents];

    while (!stop_tok.stop_requested()) {
        int nfds = epoll_wait(epoll_fd_, events, kMaxEvents, 100);
        for (int i = 0; i < nfds; ++i) {
            int fd = events[i].data.fd;
            auto it = callbacks_.find(fd);
            if (it != callbacks_.end() && it->second) {
                it->second();
            }
        }
    }
}

} // namespace minicache::net