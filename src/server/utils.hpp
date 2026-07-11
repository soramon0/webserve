#pragma once

#include "../http/status_code.hpp"

int set_nonblocking(int fd);
int epoll_instance();
int add_to_epoll(int epoll_fd, SOCKET sock, int flags);
int modify_epoll(int epoll_fd, int c, int flag);
bool isRedirect(HttpStatus& status);
