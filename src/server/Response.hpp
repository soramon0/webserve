#pragma once

#include "http/fsm.hpp"
#include "utils.hpp"
#include "logger/log.hpp"
#include <sstream>

class Client;

class Response
{
public:
    HttpStatus  status;
    std::string headers;
    std::string body;
    std::string buffer;
    size_t      offset; // bytes sent

    Response();
    ~Response();

    void build(HttpStatus status,
        Client* cl,  
        const std::string& content_type,
        const std::string& location = "")
    {
        Logger::debug("Builiding the response...");
        std::ostringstream resp;
        resp << "HTTP/1.1 " << status.toString() << "\r\n";

        if (!location.empty())
            resp << "Location: " << location << "\r\n";

        if (status >= HttpStatus::BAD_REQUEST)
            body = getErrorBody(cl, status);

        resp << "Content-Type: " << content_type << "\r\n"
                << "Content-Length: " << body.size() << "\r\n"
                << "Connection: close\r\n"
                << "\r\n"
                << body;

        buffer = resp.str();
        offset = 0;
    }
};
