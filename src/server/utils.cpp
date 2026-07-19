#include "utils.hpp"
#include "logger/log.hpp"
#include "Client.hpp"
#include <fcntl.h>
#include <sys/epoll.h>
#include <cstring>
#include <string>
#include <fstream>
#include <map>

int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);// TODO: remove get
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

std::string getExt(const std::string& path)
{
    size_t pos = path.rfind('.');
    if (pos == std::string::npos)
        return "";
    return path.substr(pos + 1);
}

std::string getMimeTypeFromConfig(const std::string& ext, const mimetype_map& types)
{
    if (ext.empty())
        return "application/octet-stream";

    mimetype_map::const_iterator it = types.begin();
    while (it != types.end())
    {
        if (it->second.count(ext))
            return it->first;
        ++it;
    }
    return ""; // TODO: make sure to deal with the case when there no types in conf
}

std::string getContentType(const std::string& path, const mimetype_map& types)
{
    if (path.empty())
        return "text/html";

    std::string ext = getExt(path);

    std::string type = getMimeTypeFromConfig(ext, types);
    if (!type.empty()) return type;

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
// TODO: fix the error by appending the file name to the root
std::string getErrorBody(Client* cl, HttpStatus status)
{
    if (cl->location && cl->location->shared_config)
    {
        std::map<int, std::string>& pages = cl->location->shared_config->error_page;
        std::map<int, std::string>::iterator it = pages.find(status.asInt());
        if (it != pages.end())
        {
            std::string error_page_file = cl->location->shared_config->root + "/" + it->second;
            Logger::debug("found custom error page: %s", it->second.c_str());
            std::ifstream file(error_page_file.c_str());
            if (!file.is_open()) {
                Logger::debug("Can't open the file %s", error_page_file.c_str());
            }
            Logger::debug("reading the custom 404...");
            std::string body((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());
            return body;
        }
    }
    // fallback to default
    return "<html><body><h1>" + std::string(status.toString()) + "</h1></body></html>";
}