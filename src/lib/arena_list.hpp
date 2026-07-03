#pragma once

#include <cstdio>

#ifndef DEFAULT_ALIGNMENT
#define DEFAULT_ALIGNMENT (2 * sizeof(void *))
#endif

class ArenaList {
private:
  struct Block {
    unsigned char *buf;
    Block *next;
    size_t capacity;
    size_t prev_offset;
    size_t curr_offset;
    size_t alignment;
    bool zeroout;
  };

  Block *head;
  Block *current;
  size_t count;
  size_t max_cap;

  ArenaList(const ArenaList &other);
  ArenaList &operator=(const ArenaList &other);

  void *grow(size_t size);

public:
  ArenaList() : head(NULL), current(NULL), count(0), max_cap(0) {};

  size_t getBlockCount() const { return count; }
  void setMaxCap(size_t cap) { max_cap = cap; }

  bool init(size_t capacity, size_t max_capacity);
  void *alloc(size_t size);
};
