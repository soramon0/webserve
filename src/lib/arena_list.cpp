#include "arena_list.hpp"
#include <new>

bool ArenaList::init(size_t capacity, size_t max_capacity) {
  if (capacity == 0 || capacity > max_capacity)
    return false;

  Block *block = reinterpret_cast<Block *>(
      new (std::nothrow) unsigned char[(sizeof(Block) + capacity)]);

  if (block == NULL) {
    return false;
  }

  this->max_cap = max_capacity;
  block->next = NULL;
  block->buf = reinterpret_cast<unsigned char *>(block) + sizeof(Block);
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

void *ArenaList::grow(size_t size) {
  if (size > max_cap)
    return NULL;

  size_t total_size = (sizeof(Block) + max_cap);
  unsigned char *mem = new (std::nothrow) unsigned char[total_size];
  Block *block = reinterpret_cast<Block *>(mem);

  if (block == NULL) {
    return NULL;
  }

  block->next = NULL;
  block->buf = reinterpret_cast<unsigned char *>(block) + sizeof(Block);
  block->capacity = max_cap;
  block->curr_offset = 0;
  block->prev_offset = 0;
  block->alignment = head->alignment;
  block->zeroout = head->zeroout;

  current->next = block;
  current = block;
  count++;

  // return block->alloc(size);
  return NULL;
}

void *ArenaList::alloc(size_t size) {
  (void)size;
  return NULL;
  // void *data = current->alloc(size);
  // if (!data) {
  // return grow(size);
  //}

  // return data;
}
