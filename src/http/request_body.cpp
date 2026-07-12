#include "request_body.hpp"
#include "common.h"
#include "logger/log.hpp"
#include <string>

RequestBody::RequestBody()
    : total_size(0), file_read_offset(0), is_file(false),
      memory_read_done(false) {
  arena.setAlignment(1);
  arena.setZeroout(false);
}

RequestBody::~RequestBody() {
  if (this->temp_file.is_open()) {
    this->temp_file.close();
  }

  if (is_file && !temp_file_path.empty()) {
    std::remove(temp_file_path.c_str());
  }
}

bool RequestBody::init(size_t size) {
  if (initialized)
    return true;

  size_t cap = static_cast<size_t>(KIB(16));
  if (size > cap) {
    if (!init_file()) {
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
    if (!init_file())
      return false;

    if (arena.consumed() > 0) {
      temp_file.write(reinterpret_cast<const char *>(arena.getInternalBuffer()),
                      arena.consumed());

      if (temp_file.fail())
        return false;
    }
  }

  if (is_file) {
    temp_file.write(buf, size);
    if (temp_file.fail())
      return false;
  } else {
    if (!arena.str_append(buf, size)) {
      return false;
    }
  }

  total_size += size;

  return true;
}

ReadResult RequestBody::read() {
  ReadResult res;
  res.block = NULL;
  if (!is_file) {
    if (memory_read_done || total_size == 0) {
      res.status = READ_DONE;
      return res;
    }
    memory_read_done = true;
    res.status = READ_DONE; // Memory mode reads everything in one shot
    res.block = &arena;
    return res;
  }

  std::ifstream file_stream;
  if (!openInputStream(file_stream)) {
    Logger::error("Failed to open temporary upload file");
    res.status = READ_ERROR;
    return res;
  }

  file_stream.seekg(file_read_offset, std::ios::beg);

  arena.deinit();
  size_t chunk_capacity = KIB(8);
  if (!arena.init(chunk_capacity) || !arena.alloc(chunk_capacity)) {
    res.status = READ_ERROR;
    return res;
  }

  char *buf = reinterpret_cast<char *>(arena.getInternalBuffer());
  file_stream.read(buf, chunk_capacity);

  if (file_stream.bad()) {
    file_stream.close();
    res.status = READ_ERROR;
    return res;
  }

  std::streamsize bytes_read = file_stream.gcount();
  file_stream.close();

  if (bytes_read <= 0) {
    res.status = READ_DONE;
    return res;
  }

  file_read_offset += bytes_read;
  if (!arena.resize(buf, chunk_capacity, bytes_read)) {
    res.status = READ_ERROR;
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
  this->memory_read_done = false;
  if (this->is_file && this->arena.usable()) {
    this->arena.deinit();
  }
}

void RequestBody::finalize() {
  if (temp_file.is_open()) {
    temp_file.close();
  }
}

bool RequestBody::init_file() {
  if (temp_file.is_open() || is_file)
    return true;

  is_file = true;
  temp_file_path = "/tmp/webserv_upload_" + to_string_patch(rand()) + ".tmp";
  temp_file.open(temp_file_path.c_str(), std::ios::binary);
  return temp_file.is_open();
}

bool RequestBody::openInputStream(std::ifstream &out_stream) const {
  if (!is_file)
    return false;

  out_stream.open(temp_file_path.c_str(), std::ios::in | std::ios::binary);
  return out_stream.is_open();
}

std::string RequestBody::to_string_patch(int n) const {
  char buf[32];
  std::sprintf(buf, "%d", n);
  return std::string(buf);
}
