#include "http_request.hpp"
#include "common.h"
#include "logger/log.hpp"
#include "request_state.hpp"

HttpRequest::HttpRequest() : ready(true), status(HttpStatus::OK) {
  arena.setAlignment(1);
  arena.setZeroout(false);
  if (!arena.init(KIB(1), KIB(8))) {
    ready = false;
  }
}

HttpRequest::~HttpRequest() {}

void HttpRequest::printRequest() const {
  Logger::debug("-------------------");
  Logger::debug("--- HttpRequest ---");

  Logger::debug("Method:\t%.*s", static_cast<int>(method_view.length()),
                method_view.data());
  Logger::debug("URI:\t%.*s", static_cast<int>(uri.length()), uri.data());
  Logger::debug("Protocal:\t%.*s", static_cast<int>(version_view.length()),
                version_view.data());

  Logger::debug("--- Headers ---");
  if (headers.size() > 0) {
    Headers::AllRange range = headers.all();
    for (Headers::AllRange::first_type it = range.first; it != range.second;
         ++it) {
      Logger::debug("%.*s: %.*s", (int)it->first.length(), it->first.data(),
                    (int)it->second.length(), it->second.data());
    }
  }

  Logger::debug("-------------------");
}

bool HttpRequest::updateField(StringView &field, const char *buf, size_t size) {
  if (size == 0)
    return true;

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

void HttpRequest::dumpState() {
  Logger::info("req.status = %d", this->status.asInt());
  Logger::info("req.error = %.*s", (int)error.length(), error.data());
  printRequest();
};
