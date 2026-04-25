#pragma once
#include <string>

struct TokenType {
  enum Type {
    HTTP,
    SERVER,
    LOCATION,
    RETURN,
    WORD,    // unquoted: example_path, root, proxy_pass
    STRING,  // quoted: "/var/www/my site/", "high-security"
    INTEGER, // 80, 443
    LEFT_BRACE,
    RIGHT_BRACE,
    SEMICOLON,
    END_OF_FILE,
  };
};

struct Token {
  TokenType::Type type;
  std::string lexeme;
  size_t line;
  size_t column;

  Token(TokenType::Type t, const std::string v, size_t l)
      : type(t), lexeme(v), line(l) {}
};
