#pragma once

#include "token.hpp"
#include <cstdio>
#include <map>
#include <string>
#include <vector>

class Scanner {
public:
  Scanner() {
    keywords["events"] = Directive::EVENTS;
    keywords["http"] = Directive::HTTP;
    keywords["server"] = Directive::SERVER;
    keywords["location"] = Directive::LOCATION;
  }

  ssize_t scan(const char *filepath);
  void printTokens() const;

private:
  std::map<std::string, Directive::Type> keywords;
  std::vector<Token> tokens;
  std::vector<std::streampos> lineOffsets;

  friend class Parser;
};
