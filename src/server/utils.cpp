#include "utils.hpp"
#include <fcntl.h>
#include <sys/epoll.h>
#include "logger/log.hpp"

int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int epoll_instance()
{
    int fd = epoll_create();

    if (fd == -1)
        Logger::fatal("Error: epoll_create failed");
    return fd;
}

void add_to_epoll(int epoll_fd, SOCKET listen_sock, int flags)
{
    struct epoll_event ev;
    ev.events = flags;
    ev.data.fd = listen_sock;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_sock, &ev) ==  -1)
        Logger::error("Error: Can't add listen_socket %d to epoll", listen_sock);
}
