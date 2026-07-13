#include "request_body.hpp"
#include "common.h"
#include "logger/log.hpp"
#include <fstream>
#include <string>

RequestBody::RequestBody()
    : total_size(0), file_read_offset(0), initialized(false), is_file(false),
      mem_read_done(false) {
  arena.setAlignment(1);
  arena.setZeroout(false);
}

RequestBody::~RequestBody() {
  closefile(writef);
  closefile(readf);

  if (is_file && !filepath.empty()) {
    std::remove(filepath.c_str());
  }
}

bool RequestBody::init(size_t size) {
  if (initialized)
    return true;

  size_t cap = static_cast<size_t>(KIB(16));
  if (size > cap) {
    if (!init_writef()) {
      return false;
    }
    if (arena.usable()) {
      return arena.deinit();
    }
    initialized = true;
    return true;
  }

  if (!arena.init(cap))
    return false;
  initialized = true;
  return true;
}

bool RequestBody::append(const char *buf, size_t size) {
  if (!is_file && (total_size + size > arena.getCapacity())) {
    if (!init_writef())
      return false;

    if (arena.consumed() > 0) {
      const char *buf = reinterpret_cast<const char *>(arena.getBuffer());
      writef.write(buf, arena.consumed());

      if (writef.fail())
        return false;
    }
  }

  if (is_file) {
    writef.write(buf, size);
    if (writef.fail())
      return false;
  } else {
    if (!arena.str_append(buf, size)) {
      return false;
    }
  }

  total_size += size;

  return true;
}

RequestBody::ReadResult RequestBody::read() {
  ReadResult res;
  res.block = NULL;
  if (!is_file) {
    if (mem_read_done || total_size == 0) {
      res.status = READ_DONE;
      return res;
    }
    mem_read_done = true;
    res.status = READ_DONE; // Memory mode reads everything in one shot
    res.block = &arena;
    return res;
  }

  if (!openfile(readf, std::ios::in)) {
    Logger::error("Failed to open temporary upload file");
    res.status = READ_ERROR;
    return res;
  }

  size_t chunk_capacity = KIB(8);
  if (!arena.setup(chunk_capacity)) {
    Logger::error("Failed to setup scratch arena");
    readf.close();
    res.status = READ_ERROR;
    return res;
  }

  arena.free_all();            // clear internal state without freeing buffer
  arena.alloc(chunk_capacity); // should never fail

  char *buf = reinterpret_cast<char *>(arena.getBuffer());
  readf.read(buf, chunk_capacity);

  if (readf.bad()) {
    readf.close();
    res.status = READ_ERROR;
    return res;
  }

  std::streamsize bytes_read = readf.gcount();

  if (bytes_read <= 0) {
    res.status = READ_DONE;
    return res;
  }

  file_read_offset += bytes_read;
  if (!arena.resize(buf, chunk_capacity, bytes_read)) {
    res.status = READ_ERROR;
    readf.close();
    return res;
  }
  res.block = &arena;
  if (file_read_offset >= total_size) {
    res.status = READ_DONE;
  } else {
    res.status = READ_CHUNK;
  }

  return res;
}

void RequestBody::resetReader() {
  this->file_read_offset = 0;
  this->mem_read_done = false;
  closefile(readf);
  if (this->is_file && this->arena.usable()) {
    this->arena.deinit();
  }
}

void RequestBody::finalize() {
  if (writef.is_open()) {
    writef.close();
  }
}

bool RequestBody::init_writef() {
  if (writef.is_open() || is_file)
    return true;

  is_file = true;
  filepath = "/tmp/webserv_upload_" + to_string_patch(rand()) + ".tmp";
  writef.open(filepath.c_str(), std::ios::binary);
  return writef.is_open();
}

std::string RequestBody::to_string_patch(int n) const {
  char buf[32];
  std::sprintf(buf, "%d", n);
  return std::string(buf);
}
