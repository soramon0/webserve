#include "request_body.hpp"
#include "common.h"
#include <string>

RequestBody::RequestBody() : total_size(0), is_file(false) {
  arena.setAlignment(1);
  arena.setZeroout(false);
}

RequestBody::~RequestBody() {
  if (this->temp_file.is_open()) {
    this->temp_file.close();
  }
}

bool RequestBody::init(size_t size) {
  if (initialized)
    return true;

  initialized = true;
  size_t cap = static_cast<size_t>(KIB(16));
  if (size > cap) {
    if (!init_file()) {
      return false;
    }
    if (arena.usable()) {
      return arena.deinit();
    }
    return true;
  }

  return arena.init(cap);
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

std::string RequestBody::to_string_patch(int n) const {
  char buf[32];
  std::sprintf(buf, "%d", n);
  return std::string(buf);
}
