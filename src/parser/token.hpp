#pragma once
#include <sstream>
#include <string>

struct Directive {
  enum Type {
    HTTP,
    SERVER,
    LOCATION,
    WORD,
    LBRACE,
    RBRACE,
    SEMICOLON,
    END_OF_FILE,
  };

  static std::string toString(Type type) {
    switch (type) {
    case HTTP:
      return "HTTP";
    case SERVER:
      return "SERVER";
    case LOCATION:
      return "LOCATION";
    case WORD:
      return "WORD";
    case LBRACE:
      return "LBRACE";
    case RBRACE:
      return "RBRACE";
    case SEMICOLON:
      return "SEMICOLON";
    case END_OF_FILE:
      return "END_OF_FILE";
    default:
      return "UNKNOWN";
    }
  }
};

inline std::ostream &operator<<(std::ostream &os, Directive::Type type) {
  return os << Directive::toString(type);
}

struct Token {
  Directive::Type type;
  std::string lexeme;
  size_t row;
  size_t column;

  Token(Directive::Type t, const std::string &v, size_t r, size_t c)
      : type(t), lexeme(v), row(r), column(c) {}
  Token(Directive::Type t, size_t r, size_t c) : type(t), row(r), column(c) {}

  std::string toString() const {
    std::ostringstream oss;
    oss << row << ":" << column << " " << type << ": ";
    if (lexeme.empty()) {
      oss << type;
    } else {
      oss << lexeme;
    }
    return oss.str();
  };
};
