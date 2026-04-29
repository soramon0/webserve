#include "parser.hpp"
#include "config/config.hpp"
#include "logger/log.hpp"
#include "parser/token.hpp"
#include <vector>

Parser::Parser(const std::vector<Token> &t) : tokens(t), pos(0) {}

Config *Parser::parse() {
  if (tokens.empty() || tokens[pos].type == Directive::END_OF_FILE) {
    Logger::error("Configuration file is empty");
    return NULL;
  }

  Config *cfg = new Config();
  return (cfg);
}
