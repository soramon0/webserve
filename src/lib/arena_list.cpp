#include "arena_list.hpp"
#include "arena.hpp"
#include <cassert>
#include <new>
#include <stdint.h>

ArenaList::~ArenaList() {
  ArenaBlock *curr = head;
  while (curr != NULL) {
    ArenaBlock *next_block = curr->next;

    curr->~ArenaBlock();
    delete[] reinterpret_cast<unsigned char *>(curr);

    curr = next_block;
  }
  head = NULL;
  current = NULL;
  count = 0;
}

ArenaBlock *ArenaList::createBlock(size_t cap) {
  size_t total = sizeof(ArenaBlock) + cap + (alignment - 1);
  unsigned char *mem = new (std::nothrow) unsigned char[total];
  if (mem == NULL) {
    return NULL;
  }

  ArenaBlock *block = new (mem) ArenaBlock(cap, alignment, zeroout);
  uintptr_t curr_ptr = reinterpret_cast<uintptr_t>(mem + sizeof(ArenaBlock));
  block->buf =
      reinterpret_cast<unsigned char *>(align_forward(curr_ptr, alignment));
  return block;
}

bool ArenaList::init(size_t capacity, size_t max_capacity) {
  if (capacity == 0 || capacity > max_capacity)
    return false;

  ArenaBlock *block = createBlock(capacity);
  if (!block)
    return false;
  head = block;
  current = block;
  max_cap = max_capacity;
  count = 1;
  return true;
};

void *ArenaList::grow(size_t size) {
  if (size > max_cap)
    return NULL;

  ArenaBlock *block = createBlock(max_cap);
  if (!block)
    return NULL;

  current->next = block;
  current = block;
  count++;
  return block->alloc(size);
}

void *ArenaList::alloc(size_t size) {
  void *data = current->alloc(size);
  if (!data) {
    return grow(size);
  }
  return data;
}
