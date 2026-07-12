#include "http_request.hpp"
#include "common.h"
#include "logger/log.hpp"
#include "request_state.hpp"
#include <limits>

const size_t HttpRequest::MaxArenaBlocks = 5;

HttpRequest::HttpRequest()
    : contentLength(0), ready(true), request_line_complete(false),
      status(HttpStatus::OK) {
  arena.setAlignment(1);
  arena.setZeroout(false);

  // MaxArenaBlocks(5) -> 1kb + 4x8kb
  if (!arena.init(KIB(1), KIB(8))) {
    ready = false;
  }
}

HttpRequest::~HttpRequest() {}

void HttpRequest::printRequest() {
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

  Logger::debug("--- body ---");
  ReadResult res;
  do {
    res = this->body.read();
    if (res.block) {
      Logger::debug("%.*s", (int)res.block->consumed(),
                    res.block->getInternalBuffer());
    }
  } while (res.status != READ_DONE);
  body.resetReader();

  Logger::debug("-------------------");
}

void HttpRequest::finishRequestLine() { request_line_complete = true; }

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

    char *str = arena.str_resize(field.data(), prev_size, buf, total);
    if (!str) {
      if (!expandArena(total)) {
        return false;
      }

      char *data = arena.str_append(field.data(), field.length());
      if (!data) {
        return false;
      }
      field = StringView(data, field.length());

      str = arena.str_resize(field.data(), prev_size, buf, total);
      if (!str) {
        return false;
      }
    }
    field = StringView(str, total);
  }
  return true;
}

bool HttpRequest::expandArena(size_t size) {
  if (!request_line_complete) {
    status = HttpStatus::URI_TOO_LONG;
    error = StringView("request-line too large");
    return false;
  }

  if (arena.getBlockCount() >= MaxArenaBlocks) {
    status = HttpStatus::REQUEST_ENTITY_TOO_LARGE;
    error = StringView("request too large");
    return false;
  }

  if (size > arena.getMaxCap()) {
    status = HttpStatus::REQUEST_HEADER_FIELDS_TOO_LARGE;
    error = StringView("header too large");
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

bool HttpRequest::validateHeaders(StringView &key, StringView &value) {
  if (key == "host") {
    if (headers.has(key)) {
      StringView("duplicate host header");
      status = HttpStatus::BAD_REQUEST;
      return false;
    }

    if (value.empty()) {
      error = StringView("host cannot be empty");
      status = HttpStatus::BAD_REQUEST;
      return false;
    }
    return true;
  }

  if (key == "connection") {
    // future feature
    // TODO: Store connection state on the request context for quick access
    // later
    if (value.empty()) {
      error = StringView("connection cannot be empty");
      status = HttpStatus::BAD_REQUEST;
      return false;
    }
    return true;
  }

  if (key == "content-length") {
    if (headers.has("transfer-encoding")) {
      error = StringView("bad content-length");
      status = HttpStatus::BAD_REQUEST;
      return false;
    }

    if (headers.has(key)) {
      error = StringView("duplicate content-length header");
      status = HttpStatus::BAD_REQUEST;
      return false;
    }

    size_t parsed_len = 0;
    if (!parseContentLength(value, parsed_len)) {
      error = StringView("content-length invalid or overflowed");
      status = HttpStatus::BAD_REQUEST;
      return false;
    }

    contentLength = parsed_len;
    return true;
  }

  if (key == "transfer-encoding") {
    if (headers.has("content-length")) {
      error = StringView("bad transfer-encoding");
      status = HttpStatus::BAD_REQUEST;
      return false;
    }
    if (headers.has(key)) {
      error = StringView("duplicate transfer-encoding header");
      status = HttpStatus::BAD_REQUEST;
      return false;
    }
    if (value.empty()) {
      error = StringView("transfer-encoding cannot be empty");
      status = HttpStatus::BAD_REQUEST;
      return false;
    }
    if (value != "chunked") {
      error = StringView("transfer-encoding only support chunked");
      status = HttpStatus::BAD_REQUEST;
      return false;
    }
    return true;
  }
  if (key == "authorization") {
    if (headers.has(key)) {
      error = StringView("duplicate authorization header");
      status = HttpStatus::BAD_REQUEST;
      return false;
    }
    if (value.empty()) {
      error = StringView("authorization cannot be empty");
      status = HttpStatus::BAD_REQUEST;
      return false;
    }
    return true;
  }
  if (key == "user-agent") {
    if (headers.has(key)) {
      StringView("duplicate user-agent header");
      status = HttpStatus::BAD_REQUEST;
      return false;
    }
    if (value.empty()) {
      StringView("user-agent cannot be empty");
      status = HttpStatus::BAD_REQUEST;
      return false;
    }
    return true;
  }
  if (key == "date") {
    if (headers.has(key)) {
      StringView("duplicate date header");
      status = HttpStatus::BAD_REQUEST;
      return false;
    }
    if (value.empty()) {
      StringView("date cannot be empty");
      status = HttpStatus::BAD_REQUEST;
      return false;
    }
    return true;
  }

  if (key == "content-type") {
    if (headers.has(key)) {
      StringView("duplicate content-type header");
      status = HttpStatus::BAD_REQUEST;
      return false;
    }
    if (value.empty()) {
      error = StringView("content-type cannot be empty");
      status = HttpStatus::BAD_REQUEST;
      return false;
    }
    return true;
  }

  return true;
}

bool HttpRequest::parseContentLength(const StringView &value,
                                     size_t &out) const {
  if (value.empty() || value.length() > 20) {
    return false;
  }

  char buf[32];
  std::memcpy(buf, value.data(), value.length());
  buf[value.length()] = '\0';

  char *endptr;
  errno = 0;
  unsigned long parsed_val = std::strtoul(buf, &endptr, 10);

  // Check if the entire string was consumed
  if (endptr == buf || *endptr != '\0') {
    return false;
  }

  // Check for general strtoul overflow (e.g., value > ULONG_MAX)
  if (errno == ERANGE) {
    return false;
  }

  if (parsed_val > std::numeric_limits<size_t>::max()) {
    return false;
  }

  out = static_cast<size_t>(parsed_val);
  return true;
}
