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
  bool request_line_complete;
  static const size_t MaxArenaBlocks;

  bool expandArena(size_t size);

public:
  ArenaList arena;
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
};
