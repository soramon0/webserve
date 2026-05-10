#pragma once
#include <sstream>
#include <string>

struct Directive {
  enum Type {
    EVENTS,
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
    case EVENTS:
      return "events";
    case HTTP:
      return "http";
    case SERVER:
      return "server";
    case LOCATION:
      return "location";
    case WORD:
      return "word";
    case LBRACE:
      return "{";
    case RBRACE:
      return "}";
    case SEMICOLON:
      return ";";
    case END_OF_FILE:
      return "EOF";
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
    oss << row << ":" << column + 1 << " " << type << ": ";
    if (lexeme.empty()) {
      oss << type;
    } else {
      oss << lexeme;
    }
    return oss.str();
  };
};
