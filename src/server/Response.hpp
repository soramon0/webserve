#pragma once

#include "http/fsm.hpp"
#include "utils.hpp"
#include "logger/log.hpp"
#include <sstream>

struct Client;

class Response
{
public:
    HttpStatus  status;
    std::string headers;
    std::string body;
    std::string buffer;
    size_t      offset; // bytes sent
    size_t      file_size;
    int         file_fd;
    bool        headers_sent;
    bool        chunked;

    Response();
    ~Response();

    void buildHeaders(const HttpRequest& req, const std::string& content_type);
    void build(HttpStatus status,
        Client* cl,
        const std::string& location = "")
    {
        std::ostringstream heads;
        heads << "HTTP/1.1 " << status.toString() << "\r\n";

        if (!location.empty())
            heads << "Location: " << location << "\r\n";

        if (status >= HttpStatus::BAD_REQUEST)
            body = getErrorBody(cl, status);

        if (body.size() > 0)
        {
            std::string content_type = getContentType(cl);
            heads << "Content-Type: " << content_type << "\r\n";
        }
                
        heads << "Content-Length: " << body.size() << "\r\n"
            << "Connection: close\r\n"
            << "\r\n";
        headers = heads.str();
        buffer = headers + body;
        offset = 0;
    }
};
