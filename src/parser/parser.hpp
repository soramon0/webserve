#pragma once

#include "config/config.hpp"
#include "parser/scanner.hpp"
#include "parser/token.hpp"
#include <cstdio>

class Parser {
public:
  Parser(const char *filepath);
  Config *parse();

private:
  enum Context { CTX_ROOT, CTX_HTTP, CTX_SERVER, CTX_LOCATION };

  std::string cfgFile;
  size_t pos;
  Context ctx;
  Scanner scanner;

  ssize_t parseLocation(Config *config);
  ssize_t parseDirective(Config *config);
  void reportParseError(const Token &token, const std::string &msg) const;

  bool atEnd() const;
  const Token &peek() const;
  const Token &advance();
  bool match(const Token &token, Directive::Type type);
};
