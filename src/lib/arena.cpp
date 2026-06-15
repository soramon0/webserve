#include "arena.hpp"

#include <cassert>
#include <cstring>
#include <new>
#include <stdint.h>

static bool is_power_of_two(uintptr_t x) { return (x & (x - 1)) == 0; }

static uintptr_t align_forward(uintptr_t ptr, size_t align) {
  uintptr_t p, a, modulo;

  assert(is_power_of_two(align));

  p = ptr;
  a = static_cast<uintptr_t>(align);
  modulo = p & (a - 1);
  if (modulo != 0) {
    p += a - modulo;
  }
  return p;
}


Arena::Arena(size_t cap)
    : buf(NULL), capacity(cap), prev_offset(0), curr_offset(0) {
  buf = new unsigned char[capacity]();
  if (buf == NULL)
    return;
}

void *Arena::alloc_align(size_t size, size_t align) {
  uintptr_t curr_ptr = reinterpret_cast<uintptr_t>(buf + curr_offset);
  uintptr_t offset = align_forward(curr_ptr, align);
  offset -= reinterpret_cast<uintptr_t>(buf); // relative offset

  if (offset + size > capacity)
    return NULL;

  void *ptr = &buf[offset];
  prev_offset = offset;
  curr_offset = offset + size;
  std::memset(ptr, 0, size);

  return ptr;
}

void *Arena::alloc(size_t size) { return alloc_align(size, DEFAULT_ALIGNMENT); }

void *Arena::resize_align(void *old_memory, size_t old_size, size_t new_size,
                          size_t align) {
  unsigned char *old_mem = static_cast<unsigned char *>(old_memory);

  assert(is_power_of_two(align));

  if (old_mem == NULL || old_size == 0) {
    return alloc_align(new_size, align);
  }

  if (old_mem < buf || old_mem >= buf + capacity) {
    assert(0 && "Memory is out of bounds of the buffer in this arena");
    return NULL;
  }

  if (buf + prev_offset == old_mem) {
    curr_offset = prev_offset + new_size;
    if (new_size < old_size) {
      std::memset(&buf[curr_offset], 0, new_size - old_size);
    }
    return old_memory;
  } else {
    void *new_mem = alloc_align(new_size, align);
    size_t copy_size = old_size < new_size ? old_size : new_size;
    std::memmove(new_mem, old_memory, copy_size);
    return new_mem;
  }
}

void *Arena::resize(void *old_memory, size_t old_size, size_t new_size) {
  return resize_align(old_memory, old_size, new_size, DEFAULT_ALIGNMENT);
}

void Arena::free_all() {
  curr_offset = 0;
  prev_offset = 0;
}
