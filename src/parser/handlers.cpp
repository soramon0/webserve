#include "parser.hpp"

ssize_t Parser::handleRoot(SharedConfig &cfg) {
  const Token &dir = previous();

  if (!consume(Directive::WORD, "expected path after `root` directive."))
    return -1;

  cfg.root = previous().lexeme;

  if (peek().type == Directive::WORD) {
    reportParseError(peek(),
                     "invalid number of arguments in `root` directive.");
    return -1;
  }

  if (!consume(Directive::SEMICOLON)) {
    reportParseError(dir, "expected ';' after `" + dir.lexeme + "`.");
    return -1;
  }

  return 0;
}

ssize_t Parser::handleIndex(SharedConfig &cfg) {
  const Token &dir = previous();

  while (consume(Directive::WORD)) {
    cfg.index.push_back(previous().lexeme);
  }

  if (cfg.index.size() == 0) {
    reportParseError(previous(),
                     "at least one argument is required in `index` directive.");
    return -1;
  }

  if (!consume(Directive::SEMICOLON)) {
    reportParseError(dir, "expected ';' after `" + dir.lexeme + "`.");
    return -1;
  }

  return 0;
}
