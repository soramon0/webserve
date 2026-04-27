#pragma once
#include <string>

// map(word, directive);

struct Directive {
  enum Type {
    HTTP,
    SERVER,
    LOCATION,
    WORD,   // unquoted: example_path, root, proxy_pass
    STRING, // quoted: "/var/www/my site/", "high-security"
    LBRACE,
    RBRACE,
    SEMICOLON,
    END_OF_FILE,
  };
};

struct Token {
  Directive::Type type;
  std::string lexeme;
  size_t line;
  size_t column;

  Token(Directive::Type t, const std::string &v, size_t l)
      : type(t), lexeme(v), line(l) {}
};
