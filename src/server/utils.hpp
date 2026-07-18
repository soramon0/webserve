#pragma once

#include "common.h"
#include "../http/status_code.hpp"
#include <cstring>
#include <string>

class Client;

int set_nonblocking(int fd);
int epoll_instance();
int add_to_epoll(int epoll_fd, SOCKET sock, int flags);
int modify_epoll(int epoll_fd, int c, int flag);
bool isRedirect(HttpStatus& status);
std::string getContentType(const std::string& path);
std::string getErrorBody(Client* cl, HttpStatus status);
