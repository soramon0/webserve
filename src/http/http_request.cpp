#include "http_request.hpp"
#include "logger/log.hpp"
#include "request_state.hpp"

HttpRequest::HttpRequest() : arena(), status(HttpStatus::OK) {
  arena.setAlignment(1);
  arena.setZeroout(false);
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

bool HttpRequest::updateField(StringView &field, const char *buf, size_t size) {
  if (field.empty()) {
    char *data = arena.str_append(buf, size);
    if (!data) {
      return false;
    }
    field = StringView(data, size);
  } else {
    size_t prev_size = field.length();
    size_t total = field.length() + size;
    char *str = arena.str_resize(field.data(), prev_size, buf, total);
    if (!str) {
      return false;
    }
    field = StringView(str, total);
  }
  return true;
}
