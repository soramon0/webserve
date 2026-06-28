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
    ctx.fsm.setMalformed400("malformed request-line");
    return stateError(ctx);
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
    ctx.fsm.setMalformed400("invalid request method");
    return stateError(ctx);
  }

  size_t size = ctx.offset - sp - start;
  if (!ctx.req->updateField(ctx.req->method_view, &ctx.buf[start], size)) {
    ctx.fsm.setMalformed500();
    return stateError(ctx);
  }

  if (!sp) {
    return stateMethod;
  }

  ctx.req->method = HttpMethod(ctx.req->method_view);
  if (ctx.req->method.isUnknown()) {
    ctx.fsm.setMalformed(HttpStatus::NOT_IMPLEMENTED, "method not implemented");
    return stateError(ctx);
  }

  if (!ctx.req->method.isSupported()) {
    ctx.fsm.setMalformed(HttpStatus::METHOD_NOT_ALLOWED, "method not allowed");
    return stateError(ctx);
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
    return stateError(ctx);
  }

  if (sp && ctx.req->uri.empty()) {
    ctx.fsm.setMalformed400("uri is required");
    return stateError(ctx);
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
      ctx.fsm.setMalformed400("uri is invalid");
      return stateError(ctx);
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

  ctx.fsm.setMalformed400("invalid uri in request-line");
  return stateError(ctx);
}

State stateVersion(Context &ctx) {
  Logger::debug("state: version");

  size_t start = ctx.offset;
  bool end = false;
  while (ctx.offset < ctx.len) {
    if (ctx.fsm.isCRLF(ctx.buf[ctx.offset])) {
      end = true;
      break;
    }

    ctx.offset++;
  }

  size_t size = ctx.offset - start;
  if (!ctx.req->updateField(ctx.req->version_view, &ctx.buf[start], size)) {
    ctx.fsm.setMalformed500();
    return stateError(ctx);
  }

  Logger::info("GOT here");
  if (!end) {
    return stateVersion;
  }

  ctx.fsm.consumeCRLF(ctx.buf, ctx.len, ctx.offset);
  if (ctx.req->version_view.empty()) {
    ctx.fsm.setMalformed400("http version is required");
    return stateError(ctx);
  }

  ctx.req->version = HttpVersion(ctx.req->version_view);
  if (ctx.req->version.isUnknown() || !ctx.req->version.isSupported()) {
    ctx.fsm.setMalformed400("http version is not supported");
    return stateError(ctx);
  }

  return stateHeaderKey;
}

State stateHeaderKey(Context &ctx) {
  ctx.offset = ctx.len;
  return stateDone(ctx);
}

State stateHeaderValue(Context &ctx) {
  ctx.offset = ctx.len;
  return stateDone(ctx);
}

State stateDone(Context &ctx) {
  Logger::debug("state: done");
  // skip to end of buffer
  ctx.offset = ctx.len;
  ctx.fsm.setDone();
  return stateDone;
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
