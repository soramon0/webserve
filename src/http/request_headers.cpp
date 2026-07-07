#include "request_headers.hpp"
#include <cctype>

/**
 * @brief Validates if a header key follows the RFC 7230 'token' grammar.
 * A valid key must contain at least one character and consist only of
 * visible alphanumeric characters or specific allowed symbols.
 */

bool Headers::isValidKeyChar(unsigned char c) {
  // Catch spaces, controls, and DEL
  if (c <= 0x20 || c == 0x7F)
    return false;

  return !(c == '(' || c == ')' || c == '<' || c == '>' || c == '@' ||
           c == ',' || c == ';' || c == ':' || c == '\\' || c == '"' ||
           c == '/' || c == '[' || c == ']' || c == '?' || c == '=' ||
           c == '{' || c == '}');
}

bool Headers::isValidKey(const StringView &key) {
  if (key.empty())
    return false;

  for (std::size_t i = 0; i < key.length(); ++i) {
    unsigned char c = static_cast<unsigned char>(key[i]);
    if (!isValidKeyChar(c)) {
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

void Headers::normalizeKey(char *buf, size_t len) {
  size_t i = 0;
  while (i < len) {
    buf[i] = std::tolower(static_cast<unsigned char>(buf[i]));
    i++;
  }
}

void Headers::normalizeValue(StringView &value) {
  if (value.empty()) {
    return;
  }

  const char *str = value.data();
  int start = 0;
  int end = static_cast<int>(value.length()) - 1;

  while (start <= end && (str[start] == ' ' || str[start] == '\t')) {
    start++;
  }

  while (end >= start && (str[end] == ' ' || str[end] == '\t')) {
    end--;
  }

  if (start > end) {
    // empty header values allowed
    value = StringView(str, 0);
    return;
  }

  std::size_t new_len = static_cast<std::size_t>(end - start + 1);
  value = StringView(str + start, new_len);
}
