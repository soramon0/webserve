#include "http_request.hpp"
#include "logger/log.hpp"

HttpRequest::HttpRequest() {}
HttpRequest::~HttpRequest() {}

void HttpRequest::printRequest() const {
  Logger::debug("-------------------");
  Logger::debug("--- HttpRequest ---");
  Logger::debug("Method   : %s", method.c_str());
  Logger::debug("Path     : %s", path.c_str());
  Logger::debug("Query    : %s", query.c_str());
  Logger::debug("Protocal : %s", protocol.c_str());

  Logger::debug("--- Headers ---");
  std::map<std::string, std::string>::const_iterator it = headers.begin();
  for (; it != headers.end(); ++it) {
    Logger::debug("%s: %s", it->first.c_str(), it->second.c_str());
  }

  if (body.length() != 0) {
    Logger::debug("--- Body ---");
    Logger::debug("%s", body.c_str());
  }
  Logger::debug("-------------------");
}
