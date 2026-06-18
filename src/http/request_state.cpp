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
  while (ctx.offset < ctx.len) {
    if (std::isspace(static_cast<unsigned char>(ctx.buf[ctx.offset]))) {
      foundSpace = 1;
      ctx.offset++;
      break;
    }
    ctx.offset++;
  }

  size_t size = (ctx.offset - foundSpace) - start;
  char *data = static_cast<char *>(ctx.req.arena.alloc(size));
  if (!data) {
    ctx.req.status = 1; // OOM
    return stateError;
  }
  std::memcpy(data, ctx.buf + start, size);

  if (foundSpace == 0) {
    return stateMethod;
  }

  // TODO: fix need to keep track of alloc between calls
  // iter 1: GE
  // iter 2: T /api HTTP/1.1
  ctx.req.method = StringView(data, size);

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
