#pragma once

#include "http/fsm.hpp"
#include "Response.hpp"

class Response
{
public:
    HttpStatus   status;
    std::string  headers;
    std::string  body;
    std::string  buffer;
    size_t       offset; // bytes sent

    Response();
    ~Response();

    void build(const HttpRequest& req,
               const std::string& content_type);
};
