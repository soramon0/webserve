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

  int foundSpace = 0;
  bool hasChar = false;
  while (ctx.offset < ctx.len) {
    if (std::isspace(static_cast<unsigned char>(ctx.buf[ctx.offset]))) {
      foundSpace = 1;
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

  size_t size = (ctx.offset - foundSpace) - start;
  char *data = ctx.req->arena.append_str(&ctx.buf[start], size);
  if (!data) {
    ctx.fsm.setMalformed500();
    return stateError;
  }

  size_t str_size = size;
  if (!ctx.req->method.empty()) {
    str_size += ctx.req->method.length();
  };
  ctx.req->method = StringView(data, str_size);

  if (foundSpace == 0) {
    return stateMethod;
  }

  return stateURI;
}

State stateURI(Context &ctx) {
  Logger::debug("state: URI");

  size_t start = ctx.offset;
  if (!ctx.req->arena.append_str(&ctx.buf[start], ctx.len)) {
    ctx.fsm.setMalformed500();
    return stateError;
  }
  ctx.offset++;
  return stateVersion;
}

State stateVersion(Context &ctx) {
  Logger::debug("state: version");
  size_t start = ctx.offset;
  Logger::debug("alloc size: %zu", ctx.len);
  if (!ctx.req->arena.append_str(&ctx.buf[start], ctx.len)) {
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
