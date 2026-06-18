#include "http_request.hpp"
#include "logger/log.hpp"
#include "request_state.hpp"

HttpRequest::HttpRequest() : arena(), http_status(HttpStatus::OK) {
  arena.setAlignment(1);
}

HttpRequest::~HttpRequest() {}

void HttpRequest::printRequest() const {
  Logger::debug("-------------------");
  Logger::debug("--- HttpRequest ---");

  Logger::debug("Method:\t%.*s", static_cast<int>(method.length()),
                method.data());
  Logger::debug("URI:\t%.*s", static_cast<int>(uri.length()), uri.data());
  Logger::debug("Protocal:\t%.*s", static_cast<int>(version.length()),
                version.data());

  Logger::debug("--- Headers ---");

  Logger::debug("-------------------");
}
