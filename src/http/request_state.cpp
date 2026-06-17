#include "request_state.hpp"
#include <cctype>
#include <cstddef>

State stateStart(Context &ctx) {
  (void)ctx;
  // TODO: to method state or to error State;
  return stateStart;
}

State stateMethod(Context &ctx) {
  size_t start = ctx.offset;

  while (ctx.offset < ctx.buf.length()) {
    if (std::isspace(ctx.buf[ctx.offset])) {
      break;
    }
    ctx.offset++;
  }

  size_t size = ctx.offset - start;
  (void)size;

  return stateURI;
}
