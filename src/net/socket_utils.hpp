#ifndef MINICACHE_NET_SOCKET_UTILS_HPP
#define MINICACHE_NET_SOCKET_UTILS_HPP

#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

namespace minicache::net {

inline void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags != -1) {
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }
}

} // namespace minicache::net

#endif // MINICACHE_NET_SOCKET_UTILS_HPP