#include "request_state.hpp"
#include "fsm.hpp"
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

  if (!hasChar && ctx.req->method.empty()) {
    ctx.fsm.setMalformed400();
    return stateError;
  }

  size_t size = ctx.offset - sp - start;
  if (ctx.req->method.empty()) {
    char *data = ctx.req->arena.str_append(&ctx.buf[start], size);
    if (!data) {
      ctx.fsm.setMalformed500();
      return stateError;
    }
    ctx.req->method = StringView(data, size);
  } else {
    size_t prev_size = ctx.req->method.length();
    size_t total = ctx.req->method.length() + size;
    char *buf = ctx.req->arena.str_resize(ctx.req->method.data(), prev_size,
                                          &ctx.buf[start], total);
    if (!buf) {
      ctx.fsm.setMalformed500();
      return stateError;
    }
    ctx.req->method = StringView(buf, total);
  }

  if (!sp) {
    return stateMethod;
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
    // TODO: add uri validation
    ctx.offset++;
  }

  size_t size = ctx.offset - sp - start;
  if (ctx.req->uri.empty()) {
    char *data = ctx.req->arena.str_append(&ctx.buf[start], size);
    if (!data) {
      ctx.fsm.setMalformed500();
      return stateError;
    }
    ctx.req->uri = StringView(data, size);
  } else {
    size_t prev_size = ctx.req->uri.length();
    size_t total = ctx.req->uri.length() + size;
    char *buf = ctx.req->arena.str_resize(ctx.req->uri.data(), prev_size,
                                          &ctx.buf[start], total);
    if (!buf) {
      ctx.fsm.setMalformed500();
      return stateError;
    }
    ctx.req->uri = StringView(buf, total);
  }

  if (sp && ctx.req->uri.empty()) {
    ctx.fsm.setMalformed400();
    return stateError;
  }

  if (!sp) {
    return stateURI;
  }

  return stateVersion;
}

State stateVersion(Context &ctx) {
  Logger::debug("state: version");
  size_t start = ctx.offset;
  Logger::debug("alloc size: %zu", ctx.len);
  if (!ctx.req->arena.str_append(&ctx.buf[start], ctx.len)) {
    ctx.fsm.setMalformed500();
    return stateError;
  }
  ctx.offset++;
  return stateVersion;
}

State stateError(Context &ctx) {
  Logger::debug("state: error");
  // skip to end of buffer
  ctx.offset = ctx.len;
  if (ctx.fsm.status.isPending()) {
    ctx.fsm.status = FSMStatus::MALFORMED;
  }
  ctx.hasError = true;
  return stateError;
}
