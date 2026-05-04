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
      if (parseHttp(*cfg) != 0) {
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

  if (!consume(Directive::LBRACE, "expected '{' after events"))
    return -1;

  while (!check(Directive::RBRACE) && !atEnd()) {
    if (!expectTokenContext(peek().type))
      return -1;
    advance();
  }

  if (!consume(Directive::RBRACE, "expected '}' to close events"))
    return -1;

  this->ctx.pop_back();
  return 0;
}

ssize_t Parser::parseHttp(Config &cfg) {
  if (!expectContext(CTX_ROOT, CTX_HTTP))
    return -1;
  this->ctx.push_back(CTX_HTTP);

  if (!consume(Directive::LBRACE, "expected '{' after http"))
    return -1;

  while (!check(Directive::RBRACE) && !atEnd()) {
    Directive::Type type = peek().type;
    if (!expectTokenContext(type))
      return -1;

    if (type == Directive::SERVER) {
      const Token &token = advance();
      Server srv;
      if (parseServer(srv) != 0)
        return -1;
      if (cfg.hasServer(srv)) {
        // TODO: add better error indicator;
        reportParseError(token, "server has a used host:port");
        return -1;
      }
      cfg.servers.push_back(srv);
    } else {
      if (parseDirective(*cfg.shared_config) != 0)
        return -1;
    }
  }

  if (!consume(Directive::RBRACE, "expected '}' to close http"))
    return -1;

  this->ctx.pop_back();
  return 0;
}

ssize_t Parser::parseServer(Server &srv) {
  if (!expectContext(CTX_HTTP, CTX_SERVER))
    return -1;
  this->ctx.push_back(CTX_SERVER);

  if (!consume(Directive::LBRACE, "expected '{' after server"))
    return -1;

  while (!check(Directive::RBRACE) && !atEnd()) {
    Directive::Type type = peek().type;
    if (!expectTokenContext(type))
      return -1;

    if (type == Directive::LOCATION) {
      advance();
      Location loc;
      if (parseLocation(loc) != 0)
        return -1;
      srv.locations[loc.path] = loc;
    } else {
      if (parseDirective(*srv.shared_config) != 0)
        return -1;
    }
  }

  if (!consume(Directive::RBRACE, "expected '}' to close server"))
    return -1;

  this->ctx.pop_back();
  return 0;
}

ssize_t Parser::parseLocation(Location &loc) {
  if (!expectContext(CTX_SERVER, CTX_LOCATION))
    return -1;
  this->ctx.push_back(CTX_LOCATION);

  if (!consume(Directive::WORD, "expected location uri"))
    return -1;

  loc.path = previous().lexeme;

  if (!consume(Directive::LBRACE, "expected '{' after location"))
    return -1;

  while (!check(Directive::RBRACE) && !atEnd()) {
    Directive::Type type = peek().type;
    if (!expectTokenContext(type))
      return -1;

    if (parseDirective(*loc.shared_config) != 0)
      return -1;
  }

  if (!consume(Directive::RBRACE, "expected '}' after location"))
    return (-1);

  this->ctx.pop_back();
  return 0;
}

ssize_t Parser::parseDirective(SharedConfig &cfg) {
  if (!consume(Directive::WORD, "invalid here."))
    return -1;

  const Token &dir = previous();
  if (dir.lexeme == "root") {
    if (!consume(Directive::WORD, "expected path after `root` directive."))
      return -1;
    cfg.root = previous().lexeme;
    if (peek().type == Directive::WORD) {
      reportParseError(peek(),
                       "invalid number of arguments in `root` directive.");
      return -1;
    }
  } else if (dir.lexeme == "index") {
    while (consume(Directive::WORD)) {
      cfg.index.push_back(previous().lexeme);
    }
    if (cfg.index.size() == 0) {
      reportParseError(
          previous(),
          "at least one argument is required in `index` directive.");
      return -1;
    }
  } else {
    reportParseError(previous(), "invalid directive.");
    return -1;
  }

  if (!consume(Directive::SEMICOLON)) {
    reportParseError(dir, "expected ';' after `" + dir.lexeme + "`.");
    return -1;
  }

  return 0;
}

const Token *Parser::consume(Directive::Type type, const std::string &msg) {
  if (check(type))
    return &advance();

  reportParseError(peek(), msg);
  return NULL;
}

const Token *Parser::consume(Directive::Type type) {
  if (check(type))
    return &advance();
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
