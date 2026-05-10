#include "parser.hpp"
#include "config/config.hpp"
#include "lib/utils.hpp"
#include "logger/log.hpp"
#include <fstream>
#include <iostream>

Parser::Parser(const char *path)
    : cfgFile(path), pos(0), scanner(), ctx(), ctxMap() {
  ctx.push_back(CTX_ROOT);

  ctxMap[Directive::EVENTS].push_back(CTX_ROOT);
  ctxMap[Directive::HTTP].push_back(CTX_ROOT);
  ctxMap[Directive::SERVER].push_back(CTX_HTTP);
  ctxMap[Directive::LOCATION].push_back(CTX_SERVER);
}

Config *Parser::parse() {
  if (scanner.scan(this->cfgFile.c_str()) != 0) {
    return NULL;
  }

  if (atEnd()) {
    Logger::error("Configuration file is empty");
    return NULL;
  }

  Config *cfg = new Config();
  while (!atEnd()) {
    if (check(Directive::EVENTS)) {
      advance();
      if (parseEvents() != 0) {
        goto cleanup;
      }
    } else if (check(Directive::HTTP)) {
      advance();
      if (parseHttp(cfg) != 0) {
        goto cleanup;
      }
    } else {
      reportParseError(peek(), "unknown directive");
      goto cleanup;
    }
  }

  return cfg;

cleanup:
  delete cfg;
  return NULL;
}

// NOTE: events directive is noop for now
ssize_t Parser::parseEvents() {
  if (!expectContext(CTX_ROOT, CTX_EVENTS)) {
    return -1;
  }
  this->ctx.push_back(CTX_EVENTS);

  if (!consume(Directive::LBRACE, "expected '{'"))
    return -1;

  while (!check(Directive::RBRACE) && !atEnd()) {
    if (!expectTokenContext(peek().type))
      return -1;
    advance();
  }

  if (!consume(Directive::RBRACE, "expected '}'"))
    return -1;

  this->ctx.pop_back();
  return 0;
}

ssize_t Parser::parseHttp(Config *cfg) {
  if (!expectContext(CTX_ROOT, CTX_HTTP))
    return -1;
  this->ctx.push_back(CTX_HTTP);

  if (!consume(Directive::LBRACE, "expected '{'"))
    return -1;

  while (!check(Directive::RBRACE) && !atEnd()) {
    Directive::Type type = peek().type;
    if (!expectTokenContext(type))
      return -1;

    if (type == Directive::SERVER) {
      advance();
      if (parseServer(cfg) != 0)
        return -1;
    } else {
      if (parseDirective(cfg) != 0)
        return -1;
    }
  }

  if (!consume(Directive::RBRACE, "expected '}'"))
    return -1;

  this->ctx.pop_back();
  return 0;
}

ssize_t Parser::parseServer(Config *cfg) {
  (void)cfg;
  if (!expectContext(CTX_HTTP, CTX_SERVER))
    return -1;
  this->ctx.push_back(CTX_SERVER);

  // parse...
  advance();

  this->ctx.pop_back();
  return 0;
}

ssize_t Parser::parseLocation(Config *cfg) {
  if (!expectContext(CTX_SERVER, CTX_LOCATION))
    return -1;
  this->ctx.push_back(CTX_LOCATION);

  if (!consume(Directive::WORD, "expected location uri"))
    return -1;

  Location loc;
  loc.withPath(previous().lexeme);
  if (cfg->shared_config) {
    loc.withSharedConfig(*cfg->shared_config);
  }

  while (!check(Directive::RBRACE) && !atEnd()) {
    Directive::Type type = peek().type;
    if (!expectTokenContext(type))
      return -1;

    if (parseDirective(cfg) != 0)
      return -1;
  }

  if (!consume(Directive::RBRACE, "expected '}'"))
    return (-1);

  this->ctx.pop_back();
  return 0;
}

ssize_t Parser::parseDirective(Config *cfg) {
  (void)cfg;
  advance();
  return 0;
}

const Token *Parser::consume(Directive::Type type, const std::string &msg) {
  if (check(type))
    return &advance();

  reportParseError(peek(), msg);
  return NULL;
}

bool Parser::check(Directive::Type type) {
  if (atEnd())
    return false;
  return peek().type == type;
}

const Token &Parser::peek() const { return scanner.tokens[pos]; }
const Token &Parser::previous() const { return scanner.tokens[pos - 1]; }

const Token &Parser::advance() {
  if (!atEnd())
    pos++;
  return previous();
}

bool Parser::atEnd() const { return peek().type == Directive::END_OF_FILE; }

void Parser::reportParseError(const Token &t, const std::string &msg) const {
  std::ifstream file(this->cfgFile.c_str(), std::ios::binary);

  if (!file) {
    Logger::error("%zu:%zu| %s", t.row, t.column, msg.c_str());
  } else {
    file.seekg(scanner.lineOffsets[t.row - 1]);

    std::string line;
    if (std::getline(file, line)) {
      reportError(line, t.row, t.column, msg);
    } else {
      Logger::error("%zu:%zu| %s", t.row, t.column, msg.c_str());
    }
  }
}
