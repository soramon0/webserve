#include "string_view.hpp"

bool StringView::operator==(const StringView &other) const {
  if (this->len != other.len)
    return false;
  if (this->buf == other.buf)
    return true;
  return std::memcmp(this->buf, other.buf, this->len) == 0;
}

bool StringView::operator<(const StringView &other) const {
  std::size_t min_len = (this->len < other.len) ? this->len : other.len;
  int cmp = std::memcmp(this->buf, other.buf, min_len);

  if (cmp != 0) {
    return cmp < 0;
  }
  return this->len < other.len;
}

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
