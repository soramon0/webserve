#include "parser.hpp"
#include <sstream>

bool Parser::expectContext(Context context, Context want) {
  if (ctx.back() == context) {
    return true;
  }

  std::ostringstream oss;

  oss << "directive `" << ctxToString(want)
      << "` is not allowed here (must be at `" << ctxToString(context)
      << "` block)";

  reportParseError(previous(), oss.str());
  return false;
}

bool Parser::expectTokenContext(Directive::Type type) {
  std::map<Directive::Type, Contexts>::iterator it = ctxMap.find(type);

  // If the directive isn't in our map, we assume it's "global" or
  // we haven't restricted it yet.
  if (it == ctxMap.end())
    return true;

  Context current = ctx.back();
  bool found = false;

  for (Contexts::const_iterator itCtx = it->second.begin();
       itCtx != it->second.end(); itCtx++) {
    if (current == *itCtx) {
      found = true;
      break;
    }
  }

  if (!found) {
    std::string msg = "directive `" + Directive::toString(type) +
                      "` is not allowed in " + ctxToString(current) +
                      " context";
    reportParseError(peek(), msg);
    return false;
  }
  return true;
}

std::string Parser::ctxToString(Context context) const {
  switch (context) {
  case CTX_ROOT:
    return "root";
  case CTX_EVENTS:
    return "events";
  case CTX_HTTP:
    return "http";
  case CTX_SERVER:
    return "server";
  case CTX_LOCATION:
    return "location";
  default:
    return "unknown";
  }
}
