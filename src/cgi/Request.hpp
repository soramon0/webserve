#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>

class Request {
private:
    std::string method;
    std::string path;
    std::string query;
    std::string protocol;
    std::string body;
    std::string contentType;

public: 
    std::string getMethod() const { return method; }
    std::string getPath() const { return path; }
    std::string getQuery() const { return query; }
    std::string getProtocol() const { return protocol; }
    std::string getBody() const { return body; }
    std::string getContentType() const { return contentType; }
};

#endif