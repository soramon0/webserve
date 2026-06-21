#include "request_state.hpp"
#include "fsm.hpp"
#include "http/http_method.hpp"
#include "http/http_version.hpp"
#include "http/status_code.hpp"
#include "http_request.hpp"
#include "logger/log.hpp"
#include <cctype>
#include <cstddef>

State stateStart(Context &ctx) {
  Logger::debug("state: start");

  if (!std::isalpha(static_cast<unsigned char>(ctx.buf[ctx.offset]))) {
    ctx.fsm.setMalformed400();
    return stateError;
  }
  return stateMethod;
}

State stateMethod(Context &ctx) {
  Logger::debug("state: method");
  size_t start = ctx.offset;

  int sp = 0;
  bool hasChar = false;
  while (ctx.offset < ctx.len) {
    if (ctx.buf[ctx.offset] == ' ') {
      sp = 1;
      ctx.offset++;
      break;
    }

    hasChar = std::isalpha(static_cast<unsigned char>(ctx.buf[ctx.offset]));
    if (!hasChar) {
      break;
    }
    ctx.offset++;
  }

  if (!hasChar && ctx.req->method_view.empty()) {
    ctx.fsm.setMalformed400();
    return stateError;
  }

  size_t size = ctx.offset - sp - start;
  if (!ctx.req->updateField(ctx.req->method_view, &ctx.buf[start], size)) {
    ctx.fsm.setMalformed500();
    return stateError;
  }

  if (!sp) {
    return stateMethod;
  }

  ctx.req->method = HttpMethod(ctx.req->method_view);
  if (ctx.req->method.isUnknown()) {
    ctx.fsm.setMalformed(HttpStatus::NOT_IMPLEMENTED);
    return stateError;
  }

  if (!ctx.req->method.isSupported()) {
    ctx.fsm.setMalformed(HttpStatus::METHOD_NOT_ALLOWED);
    return stateError;
  }

  return stateURI;
}

State stateURI(Context &ctx) {
  Logger::debug("state: URI");

  size_t start = ctx.offset;
  int sp = 0;
  while (ctx.offset < ctx.len) {
    if (ctx.buf[ctx.offset] == ' ') {
      sp = 1;
      ctx.offset++;
      break;
    }
    ctx.offset++;
  }

  size_t size = ctx.offset - sp - start;
  if (!ctx.req->updateField(ctx.req->uri, &ctx.buf[start], size)) {
    ctx.fsm.setMalformed500();
    return stateError;
  }

  if (sp && ctx.req->uri.empty()) {
    ctx.fsm.setMalformed400();
    return stateError;
  }

  if (!sp) {
    return stateURI;
  }

  // Origin-from
  if (ctx.req->uri[0] == '/') {
    return stateVersion;
  }

  // Absolute-form
  size_t scheme_offset = 0;
  if (ctx.req->uri.startsWith("https://")) {
    scheme_offset = 8;
  } else if (ctx.req->uri.startsWith("http://")) {
    scheme_offset = 7;
  }

  if (scheme_offset > 0) {
    const char *domain = ctx.req->uri.data() + scheme_offset;
    size_t remaining_len = ctx.req->uri.length() - scheme_offset;

    // Reject missing domains (e.g., just "http://")
    if (remaining_len == 0) {
      ctx.fsm.setMalformed400();
      return stateError;
    }

    const char *path =
        static_cast<const char *>(std::memchr(domain, '/', remaining_len));

    if (!path) {
      // e.g., "https://example.com"|"https://e" -> defaults to "/"
      ctx.req->uri = StringView("/", 1);
    } else {
      size_t new_len = (ctx.req->uri.data() + ctx.req->uri.length()) - path;
      ctx.req->uri = StringView(path, new_len);
    }

    return stateVersion;
  }

  ctx.fsm.setMalformed400();
  return stateError;
}

State stateVersion(Context &ctx) {
  Logger::debug("state: version");

  size_t start = ctx.offset;
  int end = 0;
  while (ctx.offset < ctx.len) {
    unsigned char c = static_cast<unsigned char>(ctx.buf[ctx.offset]);
    if (c == '\r' || c == '\n') {
      end = 1;
      break;
    }
    ctx.offset++;
  }

  size_t size = ctx.offset - start;
  if (!ctx.req->updateField(ctx.req->version_view, &ctx.buf[start], size)) {
    ctx.fsm.setMalformed500();
    return stateError;
  }

  if (!end) {
    return stateVersion;
  }

  if (ctx.buf[ctx.offset] == '\r') {
    ctx.offset++;
    if (ctx.offset < ctx.len && ctx.buf[ctx.offset] == '\n') {
      ctx.offset++;
    }
  } else if (ctx.buf[ctx.offset] == '\n') {
    ctx.offset++;
  }

  if (ctx.req->version_view.empty()) {
    ctx.fsm.setMalformed400();
    return stateError;
  }

  ctx.req->version = HttpVersion(ctx.req->version_view);
  if (ctx.req->version.isUnknown() || !ctx.req->version.isSupported()) {
    ctx.fsm.setMalformed400();
    return stateError;
  }

  // TODO: remove once we add header parsing
  ctx.fsm.status = FSMStatus::DONE;
  return stateVersion;
}

State stateError(Context &ctx) {
  Logger::debug("state: error");
  // skip to end of buffer
  ctx.offset = ctx.len;
  if (ctx.fsm.status.isPending()) {
    ctx.fsm.status = FSMStatus::MALFORMED;
  }
  return stateError;
}
