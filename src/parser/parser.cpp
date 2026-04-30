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
    const Token &token = advance();
    if (token.type == Directive::HTTP) {
      // parse http
      this->ctx = Parser::CTX_HTTP;
      // ...
      // go to previous block
      this->ctx = Parser::CTX_ROOT;
    } else if (token.type == Directive::SERVER) {
      // parse server
      this->ctx = Parser::CTX_SERVER;
      // ...
      // go to previous block
      this->ctx = Parser::CTX_HTTP;
    } else if (token.type == Directive::LOCATION) {
      this->ctx = Parser::CTX_LOCATION;

      if (parseLocation(cfg) != 0) {
        goto cleanup;
      }

      this->ctx = Parser::CTX_SERVER;
    } else {
      reportParseError(token, "unknown directive");
      goto cleanup;
    }
  }

cleanup:
  delete cfg;
  cfg = NULL;

  return (cfg);
}

ssize_t Parser::parseLocation(Config *config) {
  const Token &token = advance();
  if (!match(token, Directive::WORD)) {
    reportParseError(token, "expected location uri");
    return (-1);
  }

  Location loc;
  loc.withPath(token.lexeme);
  if (config->shared_config) {
    loc.withSharedConfig(*config->shared_config);
  }

  if (!match(peek(), Directive::LBRACE)) {
    reportParseError(token, "expected \"{\" to start location block");
    return (-1);
  }

  // match directives

  return 0;
}

ssize_t Parser::parseDirective(Config *config) {
  (void)config;
  return 0;
}

const Token &Parser::peek() const { return scanner.tokens[pos]; }

const Token &Parser::advance() {
  if (!atEnd())
    pos++;
  return scanner.tokens[pos - 1];
}

bool Parser::atEnd() const {
  return scanner.tokens[pos].type == Directive::END_OF_FILE;
}

bool Parser::match(const Token &token, Directive::Type type) {
  if (token.type == type) {
    pos++;
    return true;
  }

  return false;
}

void Parser::reportParseError(const Token &t, const std::string &msg) const {
  std::ifstream file(this->cfgFile.c_str(), std::ios::binary);

  if (!file) {
    Logger::error("%zu:%zu: %s", t.row, t.column, msg.c_str());
  } else {
    size_t row = t.row;
    if (row > scanner.lineOffsets.size()) {
      row = scanner.lineOffsets.size();
    }

    file.seekg(scanner.lineOffsets[row - 1]);

    std::string line;
    if (std::getline(file, line)) {
      reportError(line, t.row, t.column, msg);
    }
  }
}
