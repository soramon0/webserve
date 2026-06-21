#include "request_state.hpp"
#include "fsm.hpp"
#include "http/http_method.hpp"
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
    // TODO: add uri validation
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
