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

  ArenaList(const ArenaList &other);
  ArenaList &operator=(const ArenaList &other);

  ArenaBlock *createBlock(size_t cap);
  void *grow(size_t size);

public:
  ArenaList()
      : head(NULL), current(NULL), count(0), max_cap(0),
        alignment(DEFAULT_ALIGNMENT) {};
  ~ArenaList();

  size_t getBlockCount() const { return count; }
  size_t getMaxCap() const { return max_cap; }
  void setMaxCap(size_t cap) { max_cap = cap; }

  bool init(size_t capacity, size_t max_capacity);
  void *alloc(size_t size);
};
