#include "string_view.hpp"

const char *StringView::startsWith(const char *target) const {
  if (!target || len == 0)
    return NULL;

  size_t i = 0;
  while (target[i] != '\0') {
    if (i >= this->len || buf[i] != target[i]) {
      return NULL;
    }
    i++;
  }

  return buf;
}

const char *StringView::endsWith(const char *target) const {
  if (!target || len == 0)
    return NULL;

  size_t target_len = std::strlen(target);

  if (target_len > this->len)
    return NULL;

  size_t offset = this->len - target_len;
  size_t i = 0;
  while (i < target_len) {
    if (buf[offset + i] != target[i]) {
      return NULL;
    }
    i++;
  }

  return buf + offset;
}
