#pragma once

#include "token.hpp"
#include <fstream>
#include <vector>

class Scanner {
public:
  ssize_t scan(char *filepath);

private:
  std::vector<Token> tokens;

  void tokenize(std::ifstream &file);
};
