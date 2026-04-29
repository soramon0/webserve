#pragma once

#include "token.hpp"
#include <fstream>
#include <map>
#include <string>
#include <vector>

class Scanner {
public:
  std::vector<Token> tokens;

  Scanner() {
    keywords["http"] = Directive::HTTP;
    keywords["server"] = Directive::SERVER;
    keywords["location"] = Directive::LOCATION;
  }

  ssize_t tokenize(std::ifstream &file);
  void printTokens() const;

private:
  std::map<std::string, Directive::Type> keywords;
};
