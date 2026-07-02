#pragma once

#include <cstdio>
#include <new>

#ifndef DEFAULT_ALIGNMENT
#define DEFAULT_ALIGNMENT (2 * sizeof(void *))
#endif

class Arena;

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
  friend class Arena;
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

class Arena {
private:
  ArenaBlock *head;
  ArenaBlock *current;
  size_t count;
  size_t max_cap;

  Arena(const Arena &other);
  Arena &operator=(const Arena &other);

  void *grow(size_t size) {
    if (size > max_cap)
      return NULL;

    ArenaBlock *block = reinterpret_cast<ArenaBlock *>(
        new (std::nothrow) unsigned char[(sizeof(ArenaBlock) + max_cap)]);

    if (block == NULL) {
      return NULL;
    }

    block->next = NULL;
    block->buf = reinterpret_cast<unsigned char *>(block) + sizeof(ArenaBlock);
    block->capacity = max_cap;
    block->curr_offset = 0;
    block->prev_offset = 0;
    block->alignment = head->alignment;
    block->zeroout = head->zeroout;

    current->next = block;
    current = block;
    count++;

    return block->alloc(size);
  }

public:
  Arena() : head(NULL), current(NULL), count(0), max_cap(0) {};

  size_t getBlockCount() const { return count; }
  void setMaxCap(size_t cap) { max_cap = cap; }

  bool init(size_t capacity, size_t max_capacity) {
    if (capacity == 0 || capacity > max_capacity)
      return false;

    ArenaBlock *block = reinterpret_cast<ArenaBlock *>(
        new (std::nothrow) unsigned char[(sizeof(ArenaBlock) + capacity)]);

    if (block == NULL) {
      return false;
    }

    this->max_cap = max_capacity;
    block->next = NULL;
    block->buf = reinterpret_cast<unsigned char *>(block) + sizeof(ArenaBlock);
    block->capacity = capacity;
    block->curr_offset = 0;
    block->prev_offset = 0;
    block->alignment = DEFAULT_ALIGNMENT;
    block->zeroout = false;

    head = block;
    current = block;
    count = 1;
    return true;
  };

  void *alloc(size_t size) {
    void *data = current->alloc(size);
    if (!data) {
      return grow(size);
    }

    return data;
  }
};
