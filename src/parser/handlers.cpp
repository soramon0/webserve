#include "parser.hpp"

ssize_t Parser::handleRoot(DirectiveCtx &ctx) {
  const Token &dir = previous();

  if (!consume(Directive::WORD, "expected path after `root` directive."))
    return -1;

  ctx.shared->root = previous().lexeme;

  if (peek().type == Directive::WORD) {
    reportParseError(peek(),
                     "invalid number of arguments in `root` directive.");
    return -1;
  }

  if (!expectEnd(dir, Directive::SEMICOLON))
    return -1;

  return 0;
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

  if (!expectEnd(dir, Directive::SEMICOLON))
    return -1;

  return 0;
}

ssize_t Parser::handleListen(DirectiveCtx &ctx) {
  const Token &dir = previous();

  if (!consume(Directive::WORD, "expected port:host after `listen` directive."))
    return -1;

  // parse these formats
  // listen port;
  // listen host;
  // listen port:host;
  // listen !0;
  const std::string &listen = previous().lexeme;
  ctx.server->interface = listen;

  // NOTE: we have cfg config but we need to update server struct

  if (peek().type == Directive::WORD) {
    reportParseError(peek(),
                     "invalid number of arguments in `listen` directive.");
    return -1;
  }

  if (!expectEnd(dir, Directive::SEMICOLON))
    return -1;

  return 0;
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
