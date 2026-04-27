#include "scanner.hpp"
#include "../logger/log.hpp"
#include "token.hpp"
#include <cctype>
#include <cstring>
#include <fstream>
#include <string>

ssize_t Scanner::scan(char *filepath) {
  std::ifstream configFile(filepath);

  if (!configFile) {
    Logger::error("Could not open configuration file '%s': %s", filepath,
                  std::strerror(errno));
    return -1;
  }

  tokenize(configFile);
  return 0;
}

void Scanner::tokenize(std::ifstream &file) {
  size_t row = 1;
  std::string src;

  while (std::getline(file, src)) {
    Logger::debug("line %zu: %s", row, src.c_str());
    size_t len = src.length();
    size_t column = 0;
    while (column < len) {
      if (std::isspace(src[column])) {
        column++;
        continue;
      }

      if (src[column] == '{') {
        tokens.push_back(Token(Directive::LBRACE, row, column++));
      } else if (src[column] == '}') {
        tokens.push_back(Token(Directive::RBRACE, row, column++));
      } else if (src[column] == ';') {
        tokens.push_back(Token(Directive::SEMICOLON, row, column++));
      } else if (src[column] == '"' || src[column] == '\'') {
        char quote = src[column];
        size_t start = ++column; // start after opening quote
        while (column < len && src[column] != quote) {
          column++;
        }
        std::string str = src.substr(start, column - start);
        tokens.push_back(Token(Directive::WORD, str, row, start - 1));
        column++; // skip closing quote
      } else {
        size_t start = column;
        while (column < len && !std::isspace(src[column]) &&
               src[column] != '{' && src[column] != '}' && src[column] != ';') {
          column++;
        }

        std::string word = src.substr(start, column - start);
        Directive::Type type = Directive::WORD;

        if (word == "http")
          type = Directive::HTTP;
        else if (word == "server")
          type = Directive::SERVER;
        else if (word == "location")
          type = Directive::LOCATION;

        tokens.push_back(Token(type, word, row, start));
      }
    }
    row++;
  }
}
