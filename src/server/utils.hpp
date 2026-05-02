#pragma once

int set_nonblocking(int fd);
int epoll_instance();
void add_to_epoll(int epoll_fd, SOCKET listen_sock, int flags);