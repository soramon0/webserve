#include "parser.hpp"
#include "config/config.hpp"
#include "lib/utils.hpp"
#include "logger/log.hpp"
#include <fstream>
#include <iostream>

Parser::Parser(const char *path) : cfgFile(path), pos(0), scanner(), ctx() {
  ctx.push_back(CTX_ROOT);
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

cleanup:
  delete cfg;
  cfg = NULL;

  return (cfg);
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
    advance();
  }

  if (!consume(Directive::RBRACE, "expected '}'"))
    return -1;

  this->ctx.pop_back();
  return 0;
}

ssize_t Parser::parseHttp(Config *cfg) {
  if (!expectContext(CTX_ROOT, CTX_HTTP)) {
    return -1;
  }
  this->ctx.push_back(CTX_HTTP);

  if (!consume(Directive::LBRACE, "expected '{'"))
    return -1;

  while (!check(Directive::RBRACE) && !atEnd()) {
    const Token &t = advance();
    if (t.type == Directive::SERVER) {
      if (parseServer(cfg) != 0)
        return -1;
    } else {
      // Handle other http-level directives like 'index'
      if (parseDirective(cfg) != 0) {
        return -1;
      }
    }
  }

  if (!consume(Directive::RBRACE, "expected '}'"))
    return -1;

  this->ctx.pop_back();
  return 0;
}

ssize_t Parser::parseServer(Config *cfg) {
  (void)cfg;
  if (!expectContext(CTX_HTTP, CTX_SERVER)) {
    return -1;
  }
  this->ctx.push_back(CTX_SERVER);

  // parse...

  this->ctx.pop_back();
  return 0;
}

ssize_t Parser::parseLocation(Config *cfg) {
  if (!expectContext(CTX_SERVER, CTX_LOCATION)) {
    return -1;
  }
  this->ctx.push_back(CTX_LOCATION);

  const Token *token = consume(Directive::WORD, "expected location uri");
  if (!token) {
    return (-1);
  }

  Location loc;
  loc.withPath(token->lexeme);
  if (cfg->shared_config) {
    loc.withSharedConfig(*cfg->shared_config);
  }

  if (!consume(Directive::LBRACE, "expected \"{\" to start location block")) {
    return (-1);
  }

  this->ctx.pop_back();
  return 0;
}

ssize_t Parser::parseDirective(Config *cfg) {
  (void)cfg;
  return 0;
}

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

const Token *Parser::consume(Directive::Type type, const std::string &msg) {
  if (check(type)) {
    return &advance();
  }

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
  if (!atEnd()) {
    pos++;
  }
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
