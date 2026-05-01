#include "parser.hpp"
#include "config/config.hpp"
#include "lib/utils.hpp"
#include "logger/log.hpp"
#include <fstream>
#include <iostream>

Parser::Parser(const char *filepath)
    : cfgFile(filepath), pos(0), ctx(Parser::CTX_ROOT), scanner() {}

Config *Parser::parse() {
  if (scanner.scan(this->cfgFile.c_str()) != 0) {
    return (NULL);
  }

  if (atEnd()) {
    Logger::error("Configuration file is empty");
    return NULL;
  }

  Config *cfg = new Config();
  while (!atEnd()) {
    bool hasErr = false;
    const Token &token = advance();

    if (token.type == Directive::HTTP) {
      hasErr = parseHttp(cfg) != 0;
    } else if (token.type == Directive::SERVER) {
      hasErr = parseServer(cfg) != 0;
    } else if (token.type == Directive::LOCATION) {
      hasErr = parseLocation(cfg) != 0;
    } else {
      reportParseError(token, "unknown directive");
      hasErr = true;
    }

    if (hasErr) {
      delete cfg;
      return (NULL);
    }
  }

  return (cfg);
}

ssize_t Parser::parseHttp(Config *config) {
  if (!ctxIs(CTX_ROOT, CTX_HTTP)) {
    return -1;
  }

  (void)config;
  return 0;
}

ssize_t Parser::parseServer(Config *config) {
  if (!ctxIs(CTX_HTTP, CTX_SERVER)) {
    return -1;
  }
  (void)config;
  return 0;
}

ssize_t Parser::parseLocation(Config *config) {
  if (!ctxIs(CTX_SERVER, CTX_LOCATION)) {
    return -1;
  }

  const Token *token = consume(Directive::WORD, "expected location uri");
  if (!token) {
    return (-1);
  }

  Location loc;
  loc.withPath(token->lexeme);
  if (config->shared_config) {
    loc.withSharedConfig(*config->shared_config);
  }

  if (!consume(Directive::LBRACE, "expected \"{\" to start location block")) {
    return (-1);
  }

  // match directives

  return 0;
}

ssize_t Parser::parseDirective(Config *config) {
  (void)config;
  return 0;
}

bool Parser::ctxIs(Context want, Context next) {
  if (this->ctx == want) {
    this->ctx = next;
    return true;
  }

  std::ostringstream oss;

  oss << "directive `" << ctxToString(next)
      << "` is not allowed here (must be at `" << ctxToString(want)
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
    Logger::error("%zu:%zu: %s", t.row, t.column, msg.c_str());
  } else {
    file.seekg(scanner.lineOffsets[t.row - 1]);

    std::string line;
    if (std::getline(file, line)) {
      reportError(line, t.row, t.column, msg);
    } else {
      Logger::error("%zu:%zu: %s", t.row, t.column, msg.c_str());
    }
  }
}

std::string Parser::ctxToString(Context context) const {
  switch (context) {
  case CTX_ROOT:
    return "root";
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
