#pragma once

#include "http/http_method.hpp"
#include "http/http_version.hpp"
#include "http/request_headers.hpp"
#include "http/status_code.hpp"
#include "lib/arena_list.hpp"
#include "lib/string_view.hpp"
#include "request_state.hpp"

class HttpRequest {
private:
  bool ready;
  bool done;
  bool request_line_complete;
  static const size_t MaxArenaBlocks;
  ArenaList arena;
  size_t contentLength;

  bool expandArena(size_t size);
  bool parseContentLength(const StringView &value, size_t &out_length) const;

public:
  ArenaList body;
  HttpStatus status;
  HttpMethod method;
  HttpVersion version;

  StringView method_view;
  StringView uri;
  StringView version_view;
  StringView error;

  Headers headers;

  HttpRequest();
  ~HttpRequest();

  void printRequest() const;
  bool updateField(StringView &field, const char *buf, size_t size);
  void finishRequestLine();
  void dumpState();
  bool getStateReady() const { return ready; }
  bool isDone() const { return done; }
  void setDone() { done = true; }
  size_t getContentLength() const { return contentLength; }
  bool validateHeaders(StringView &key, StringView &value);
};
