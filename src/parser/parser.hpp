#pragma once

#include "config/config.hpp"
#include "parser/scanner.hpp"
#include "parser/token.hpp"
#include <cstdio>

struct DirectiveCtx {
  SharedConfig *shared;
  Server *server;
  Location *loc;

  static DirectiveCtx withSharedConfig(SharedConfig *cfg);
  static DirectiveCtx withServer(Server *srv);
  static DirectiveCtx withLocation(Location *loc);
};

class Parser {
public:
  Parser(const char *path);
  Config *parse();

private:
  enum Context { CTX_ROOT, CTX_EVENTS, CTX_HTTP, CTX_SERVER, CTX_LOCATION };

  std::string cfgFile;
  size_t pos;
  Scanner scanner;
  std::vector<Context> ctx;

  typedef std::vector<Context> Contexts;
  std::map<Directive::Type, Contexts> ctxMap;

  typedef ssize_t (Parser::*DirectiveHandler)(DirectiveCtx &);
  std::map<std::string, DirectiveHandler> directiveHandlers;

  ssize_t parseEvents();
  ssize_t parseHttp(Config &cfg);
  ssize_t parseServer(Server &srv);
  ssize_t parseLocation(Location &loc);
  ssize_t parseDirective(DirectiveCtx &ctx);
  void reportParseError(const Token &token, const std::string &msg) const;

  bool atEnd() const;
  bool check(Directive::Type type);
  const Token &peek() const;
  const Token &previous() const;
  const Token &advance();
  const Token *consume(Directive::Type type, const std::string &msg);
  const Token *consume(Directive::Type type);

  bool expectContext(Context context, Context want);
  bool expectTokenContext(Directive::Type type);
  bool expectEnd(const Token &dir, Directive::Type type);

  std::string ctxToString(Context context) const;

  ssize_t handleRoot(DirectiveCtx &ctx);
  ssize_t handleIndex(DirectiveCtx &ctx);
  ssize_t handleListen(DirectiveCtx &ctx);
};
