#include "request_headers.hpp"

/**
 * @brief Validates if a header key follows the RFC 7230 'token' grammar.
 * A valid key must contain at least one character and consist only of
 * visible alphanumeric characters or specific allowed symbols.
 */
bool Headers::isValidKey(const StringView &key) {
  if (key.empty())
    return false;

  for (std::size_t i = 0; i < key.length(); ++i) {
    unsigned char c = static_cast<unsigned char>(key[i]);

    // Catch spaces, controls, and DEL
    if (c <= 0x20 || c == 0x7F)
      return false;

    if (c == '(' || c == ')' || c == '<' || c == '>' || c == '@' || c == ',' ||
        c == ';' || c == ':' || c == '\\' || c == '"' || c == '/' || c == '[' ||
        c == ']' || c == '?' || c == '=' || c == '{' || c == '}') {
      return false;
    }
  }
  return true;
}

/**
 * @brief Validates if a header value follows the RFC 7230 'field-value'
 * grammar. Enforces that the value contains no invalid control characters and
 * that all surrounding Optional Whitespace (OWS) has been properly trimmed.
 */
bool Headers::isValidValue(const StringView &value) {
  if (value.empty())
    return true;

  unsigned char first = static_cast<unsigned char>(value[0]);
  unsigned char last = static_cast<unsigned char>(value[value.length() - 1]);

  if (first == 0x20 || first == 0x09 || last == 0x20 || last == 0x09) {
    return false;
  }

  for (std::size_t i = 0; i < value.length(); ++i) {
    unsigned char c = static_cast<unsigned char>(value[i]);

    if ((c < 0x20 && c != 0x09) || c == 0x7F) {
      return false;
    }
  }
  return true;
}
