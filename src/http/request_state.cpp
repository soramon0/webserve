#include "request_state.hpp"
#include "http_request.hpp"
#include <cctype>
#include <cstddef>

State stateStart(Context &ctx) {
  (void)ctx;
  // TODO: to method state or to error State;
  return stateMethod;
}

State stateMethod(Context &ctx) {
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

  if (!hasChar && ctx.req.method.empty()) {
    ctx.req.status = 400; // Bad Request
    return stateError;
  }

  size_t size = (ctx.offset - foundSpace) - start;
  char *data = ctx.req.arena->append_str(ctx.buf + start, size);
  if (!data) {
    ctx.req.status = 1; // OOM
    return stateError;
  }

  size_t method_size = ctx.req.method.length();
  if (!ctx.req.method.empty()) {
    method_size += size;
  }
  ctx.req.method = StringView(data, method_size);

  if (foundSpace == 0) {
    return stateMethod;
  }

  return stateURI;
}

State stateURI(Context &ctx) {
  ctx.offset++;
  return stateVersion;
}

State stateVersion(Context &ctx) {
  ctx.offset++;
  return stateVersion;
}

State stateError(Context &ctx) {
  // skip to end of buffer
  ctx.offset = ctx.len;
  ctx.hasError = true;
  return stateError;
}
