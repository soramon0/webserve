#include "Response.hpp"

Response::Response() : status(HttpStatus::OK), offset(0),
                        file_size(0), file_fd(-1),
                        headers_sent(0), chunked(0) {}

Response::~Response() {}

void Response::buildHeaders(const HttpRequest& req, const std::string& content_type)
{
    std::ostringstream stream;
    stream << "HTTP/1.1 " << req.status.toString() << "\r\n"
           << "Content-Type: " << content_type << "\r\n"
           << "Content-Length: " << file_size << "\r\n"
           << "Connection: close\r\n"
           << "\r\n";
    headers = stream.str();
}