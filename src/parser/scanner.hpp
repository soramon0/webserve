#pragma once

#include "token.hpp"
#include <fstream>
#include <vector>

class Scanner {
public:
  std::vector<Token> tokens;
  ssize_t scan(char *filepath);

private:

  void tokenize(std::ifstream &file);
};
