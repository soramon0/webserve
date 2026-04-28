#pragma once

#include "token.hpp"
#include <fstream>
#include <map>
#include <vector>

class Scanner {
public:
  std::map<std::string, Directive::Type> keywords;
  std::vector<Token> tokens;

  Scanner() {
    keywords["http"] = Directive::HTTP;
    keywords["server"] = Directive::SERVER;
    keywords["location"] = Directive::LOCATION;
  }

  ssize_t scan(char *filepath);

private:
  ssize_t tokenize(std::ifstream &file);
};
