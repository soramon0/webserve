#include "common.h"
#include "utils.hpp"
#include "logger/log.hpp"
#include <fcntl.h>
#include <sys/epoll.h>
#include <cstring>
#include <string>

int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int epoll_instance()
{
    int fd = epoll_create(1);

    if (fd == -1)
        Logger::fatal("Error: epoll_create failed");
    return fd;
}

int add_to_epoll(int epoll_fd, SOCKET sock, int flags)
{
    struct epoll_event ev;

    std::memset(&ev, 0, sizeof(ev));
    ev.events = flags;
    ev.data.fd = sock;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock, &ev) ==  -1)
    {
        Logger::error("Error: Can't add socket %d to epoll", sock);
        close(sock);
        return -1;
    }
    return 0;
}

int modify_epoll(int epoll_fd, int c, int flag)
{
	struct epoll_event ev;

    std::memset(&ev, 0, sizeof(ev));
	ev.events = flag;
	ev.data.fd = c;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, c, &ev) == -1)
    {
        Logger::error("Error: Can't modify socket %d", c);
        close(c);
        return -1;
    }
    return 0;
}

bool isRedirect(HttpStatus& status) {
  return (
    status == HttpStatus::MOVED_PERMANENTLY
    || status == HttpStatus::FOUND
    || status == HttpStatus::TEMPORARY_REDIRECT
    || status == HttpStatus::PERMANENT_REDIRECT
  );
}

std::string getContentType(const std::string& path)
{
    size_t pos = path.rfind('.');
    if (pos == std::string::npos)
        return "application/octet-stream";
    std::string ext = path.substr(pos + 1); // handle if last '.' + 1 = '\0'

    if (ext == "css") return "text/css";
    if (ext == "csv") return "text/csv";
    if (ext == "gif") return "image/gif";
    if (ext == "htm") return "text/html";
    if (ext == "html") return "text/html";
    if (ext == "ico") return "image/x-icon";
    if (ext == "jpeg") return "image/jpeg";
    if (ext == "jpg") return "image/jpeg";
    if (ext == "js") return "application/javascript";
    if (ext == "json") return "application/json";
    if (ext == "png") return "image/png";
    if (ext == "pdf") return "application/pdf";
    if (ext == "svg") return "image/svg+xml";
    if (ext == "txt") return "text/plain";

    return "application/octet-stream";
}
