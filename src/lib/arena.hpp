#pragma once

#include <cstdio>

#ifndef DEFAULT_ALIGNMENT
#define DEFAULT_ALIGNMENT (2 * sizeof(void *))
#endif

class Arena {
private:
  unsigned char *buf;
  size_t capacity;
  size_t prev_offset;
  size_t curr_offset;
  size_t alignment;

  Arena(const Arena &other);
  Arena &operator=(const Arena &other);

public:
  Arena()
      : buf(NULL), capacity(0), prev_offset(0), curr_offset(0),
        alignment(DEFAULT_ALIGNMENT) {};
  ~Arena();

  void setAlignment(size_t align);
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
};
