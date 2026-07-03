#pragma once

#include <cstdio>

#ifndef DEFAULT_ALIGNMENT
#define DEFAULT_ALIGNMENT (2 * sizeof(void *))
#endif

class ArenaBlock {
private:
  unsigned char *buf;
  ArenaBlock *next;
  size_t capacity;
  size_t prev_offset;
  size_t curr_offset;
  size_t alignment;
  bool zeroout;

  ArenaBlock(const ArenaBlock &other);
  ArenaBlock &operator=(const ArenaBlock &other);

  size_t next_offset(size_t align) const;

public:
  ArenaBlock()
      : buf(NULL), next(NULL), capacity(0), prev_offset(0), curr_offset(0),
        alignment(DEFAULT_ALIGNMENT), zeroout(true) {};
  ~ArenaBlock();

  void setAlignment(size_t align);
  void setZeroout(bool val);
  bool setup(size_t cap);
  bool init(size_t cap);
  bool deinit();
  bool reinit(size_t cap);
  void *alloc_align(size_t size, size_t align);
  void *alloc(size_t size);
  void free_all();
  void *resize_align(void *old_memory, size_t old_size, size_t new_size,
                     size_t align);
  void *resize(void *old_memory, size_t old_size, size_t new_size);

  bool usable() const;
  size_t consumed() const;
  size_t available() const;

  char *str_append(const char *str, size_t len);
  char *str_resize(const char *old_memory, size_t old_size, const char *src,
                   size_t new_size);
};
