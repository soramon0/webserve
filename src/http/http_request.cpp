#include "http_request.hpp"
#include "common.h"
#include "logger/log.hpp"
#include "request_state.hpp"
#include <cstddef>

HttpRequest::HttpRequest() : state(stateStart), status(0) {}

HttpRequest::~HttpRequest() {}

bool HttpRequest::feedChunk(const char *buf, size_t len) {
  if (!arena.setup(KIB(1))) {
    status = 1; // OOM
    return false;
  }

  void *chunk = arena.alloc(len);
  if (!chunk) {
    status = 1; // OOM
    return false;
  }
  std::memmove(chunk, buf, len);

  size_t offset = 0;

  while (offset < len) {
    offset++;
  }

  return true;
}

bool HttpRequest::finish() const { return true; }

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
