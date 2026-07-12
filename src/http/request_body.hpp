#pragma once
#include "lib/arena.hpp"
#include <fstream>

class RequestBody {
private:
  ArenaBlock arena;
  std::ofstream temp_file;
  std::string temp_file_path;
  size_t total_size;
  bool is_file;
  bool initialized;
  std::string to_string_patch(int n) const;
  bool init_file();

public:
  RequestBody();
  ~RequestBody();

  bool init(size_t size);
  size_t size() const { return total_size; };
  bool isFile() const { return is_file; }
  const unsigned char *getMemoryBuffer() const {
    return arena.getInternalBuffer();
  }
  const std::string &getTempFilePath() const { return temp_file_path; }
  bool append(const char *buf, size_t size);
  void finalize();

  bool openInputStream(std::ifstream &out_stream) const {
    if (!is_file)
      return false;

    out_stream.open(temp_file_path.c_str(), std::ios::in | std::ios::binary);
    return out_stream.is_open();
  }

  size_t read();
};
