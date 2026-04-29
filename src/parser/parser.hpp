#pragma once

#include "config/config.hpp"
#include "parser/token.hpp"
#include <cstdio>
#include <vector>

class Parser {
public:
  Parser(const std::vector<Token> &tokens);
  Config *parse();

private:
  const std::vector<Token> &tokens;
  size_t pos;
};
