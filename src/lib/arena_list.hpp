#pragma once

#include "arena.hpp"
#include <cstdio>

class ArenaList {
private:
  ArenaBlock *head;
  ArenaBlock *current;
  size_t count;
  size_t max_cap;
  size_t alignment;
  bool zeroout;

  ArenaList(const ArenaList &other);
  ArenaList &operator=(const ArenaList &other);

  ArenaBlock *createBlock(size_t cap);

public:
  ArenaList()
      : head(NULL), current(NULL), count(0), max_cap(0),
        alignment(DEFAULT_ALIGNMENT), zeroout(false) {};
  ~ArenaList();

  size_t getBlockCount() const { return count; }
  size_t getBlockLeftSpace() const {
    return current ? current->available() : 0;
  }
  size_t getMaxCap() const { return max_cap; }
  void setMaxCap(size_t cap) { max_cap = cap; }
  void setAlignment(size_t align) {
    alignment = align;
    if (current) {
      current->setAlignment(align);
    }
  };
  void setZeroout(bool val) {
    zeroout = val;
    if (current) {
      current->setZeroout(val);
    }
  };

  bool init(size_t capacity, size_t max_capacity);
  void *grow(size_t size);
  void *alloc(size_t size);

  char *str_append(const char *str, size_t len) {
    return current->str_append(str, len);
  }
  char *str_resize(const char *old_memory, size_t old_size, const char *src,
                   size_t new_size) {
    return current->str_resize(old_memory, old_size, src, new_size);
  }
};
