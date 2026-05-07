#include "parser.hpp"
#include <cstdlib>

ssize_t Parser::handleRoot(DirectiveCtx &ctx) {
  const Token &dir = previous();

  if (!consume(Directive::WORD, "expected path after `root` directive."))
    return -1;

  ctx.shared->root = previous().lexeme;

  return expectDirectiveArgsCount(dir);
}

ssize_t Parser::handleIndex(DirectiveCtx &ctx) {
  const Token &dir = previous();

  while (consume(Directive::WORD)) {
    ctx.shared->index.push_back(previous().lexeme);
  }

  if (ctx.shared->index.size() == 0) {
    reportParseError(previous(),
                     "at least one argument is required in `index` directive.");
    return -1;
  }

  return expectEnd(dir, Directive::SEMICOLON) ? 0 : -1;
}

ssize_t Parser::handleListen(DirectiveCtx &ctx) {
  const Token &dir = previous();

  if (this->ctx.back() != CTX_SERVER) {
    reportParseError(dir, "`listen` directive is not allowed here.");
    return -1;
  }

  if (!consume(Directive::WORD, "expected host:port after `listen` directive."))
    return -1;

  std::string raw = previous().lexeme;
  std::string host = "0.0.0.0";
  int port = 8080;

  size_t colonPos = raw.find(':');
  if (colonPos != std::string::npos) {
    // Format: host:port
    host = raw.substr(0, colonPos);
    std::string portStr = raw.substr(colonPos + 1);
    port = std::atoi(portStr.c_str());
  } else {
    // Format could be just "8080" (port) or "localhost" (host)
    if (raw.find_first_not_of("0123456789") == std::string::npos) {
      port = std::atoi(raw.c_str());
    } else {
      host = raw;
    }
  }

  if (port <= 0 || port > 65535) {
    reportParseError(previous(), "invalid port in `listen` directive.");
    return -1;
  }

  ctx.server->interface = host;
  ctx.server->port = port;

  return expectDirectiveArgsCount(dir);
}

ssize_t Parser::handleReturn(DirectiveCtx &ctx) {
  const Token &dir = previous();

  if (this->ctx.back() != CTX_SERVER && this->ctx.back() != CTX_LOCATION) {
    reportParseError(dir, "`return` directive is not allowed here.");
    return -1;
  }

  if (!consume(Directive::WORD,
               "expected [code?] [uri] after `return` directive."))
    return -1;

  int code = 0;
  std::string uri;
  std::string raw = previous().lexeme;
  if (raw.find_first_not_of("0123456789") == std::string::npos) {
    code = std::atoi(raw.c_str());
    if (code < 300 || code >= 400) {
      reportParseError(previous(), "invalid return code.");
      return -1;
    }
  } else {
    uri = raw;
    code = 302;
  }

  if (uri.empty()) {
    if (!consume(Directive::WORD,
                 "expected [uri] after code in `return` directive."))
      return -1;
    uri = previous().lexeme;
  }

  if (this->ctx.back() == CTX_SERVER) {
    ctx.server->withRedirect(code, uri);
  }
  if (this->ctx.back() == CTX_LOCATION) {
    ctx.loc->withRedirect(code, uri);
  }

  return expectDirectiveArgsCount(dir);
}

DirectiveCtx DirectiveCtx::withServer(Server *srv) {
  DirectiveCtx ctx;
  ctx.server = srv;
  ctx.loc = NULL;
  ctx.shared = srv->shared_config;
  return ctx;
}

DirectiveCtx DirectiveCtx::withLocation(Location *loc) {
  DirectiveCtx ctx;
  ctx.server = NULL;
  ctx.loc = loc;
  ctx.shared = loc->shared_config;
  return ctx;
}

DirectiveCtx DirectiveCtx::withSharedConfig(SharedConfig *cfg) {
  DirectiveCtx ctx;
  ctx.server = NULL;
  ctx.loc = NULL;
  ctx.shared = cfg;
  return ctx;
}
