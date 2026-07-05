#include "http_request.hpp"
#include "common.h"
#include "logger/log.hpp"
#include "request_state.hpp"

HttpRequest::HttpRequest()
    : ready(true), max_arena_blocks(5), status(HttpStatus::OK) {
  arena.setAlignment(1);
  arena.setZeroout(false);
  // max_arena_blocks(5) -> 1kb + 4x8kb
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
    if (size > arena.getBlockLeftSpace() && !expandArena(size)) {
      return false;
    }

    char *data = arena.str_append(buf, size);
    if (!data) {
      return false;
    }
    field = StringView(data, size);
  } else {
    size_t prev_size = field.length();
    size_t total = field.length() + size;

    if (total > arena.getBlockLeftSpace()) {
      if (!expandArena(total)) {
        return false;
      }

      char *data = arena.str_append(field.data(), field.length());
      if (!data) {
        return false;
      }
      field = StringView(data, field.length());
    }

    char *str = arena.str_resize(field.data(), prev_size, buf, total);
    if (!str) {
      return false;
    }
    field = StringView(str, total);
  }
  return true;
}

bool HttpRequest::expandArena(size_t size) {
  if (arena.getBlockCount() > max_arena_blocks) {
    status = HttpStatus::REQUEST_ENTITY_TOO_LARGE;
    error = StringView("request is too large");
    return false;
  }

  if (size > arena.getMaxCap()) {
    if (version_view.empty()) {
      status = HttpStatus::URI_TOO_LONG;
      error = StringView("request-line too large");
    } else {
      status = HttpStatus::REQUEST_HEADER_FIELDS_TOO_LARGE;
      error = StringView("header too large");
    }
    return false;
  }

  if (!arena.assignBlock()) {
    status = HttpStatus::INTERNAL_SERVER_ERROR;
    error = StringView("internal server error");
    return false;
  }

  return true;
}

void HttpRequest::dumpState() {
  Logger::info("req.status = %d", this->status.asInt());
  Logger::info("req.error = %.*s", (int)error.length(), error.data());
  printRequest();
};
