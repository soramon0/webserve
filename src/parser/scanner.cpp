#include "scanner.hpp"
#include "../logger/log.hpp"
#include "lib/utils.hpp"
#include "token.hpp"
#include <cctype>
#include <cstdlib>
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

  return (tokenize(configFile));
}

ssize_t Scanner::tokenize(std::ifstream &file) {
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

      if (src[column] == '#') {
        column = len; // skip the whole line
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
        size_t start = ++column; // skip starting quote
        while (column < len && src[column] != quote) {
          column++;
        }

        if (column == len) {
          reportError(src, row, start - 1, "could not find closing quote");
          return -1;
        }

        std::string str = src.substr(start, column - start);
        tokens.push_back(Token(Directive::WORD, str, row, start - 1));
        column++; // Safely skip closing quote
      } else {
        size_t start = column;
        while (column < len && !std::isspace(src[column]) &&
               src[column] != '{' && src[column] != '}' && src[column] != ';') {
          column++;
        }

        std::string word = src.substr(start, column - start);
        Directive::Type type = Directive::WORD;

        std::map<std::string, Directive::Type>::iterator it =
            keywords.find(word);
        if (it != keywords.end())
          type = it->second;

        tokens.push_back(Token(type, word, row, start));
      }
    }
    row++;
  }

  tokens.push_back(Token(Directive::END_OF_FILE, row, 0));
  return (0);
}
