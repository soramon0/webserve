#pragma once

#include "config/config.hpp"
#include "parser/scanner.hpp"
#include "parser/token.hpp"
#include <cstdio>

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

  ssize_t parseEvents();
  ssize_t parseHttp(Config *cfg);
  ssize_t parseServer(Config *cfg);
  ssize_t parseLocation(Config *cfg);
  ssize_t parseDirective(Config *cfg);
  void reportParseError(const Token &token, const std::string &msg) const;

  bool atEnd() const;
  bool check(Directive::Type type);
  const Token &peek() const;
  const Token &previous() const;
  const Token &advance();
  const Token *consume(Directive::Type type, const std::string &msg);
  bool expectContext(Context context, Context want);
  bool expectTokenContext(Directive::Type type);

  std::string ctxToString(Context context) const;
};
