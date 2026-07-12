#pragma once
#include "lib/arena.hpp"
#include <fstream>

enum ReadStatus {
  READ_CHUNK, // Successfully read a chunk, call read() again
  READ_DONE,  // Reached end of file (EOF) cleanly
  READ_ERROR  // An internal file I/O error occurred
};

struct ReadResult {
  ReadStatus status;
  ArenaBlock *block;
};

class RequestBody {
private:
  ArenaBlock arena;
  std::ofstream temp_file;
  std::string temp_file_path;
  size_t total_size;
  size_t file_read_offset;
  bool initialized;
  bool is_file;
  bool memory_read_done;

  std::string to_string_patch(int n) const;
  bool init_file();
  bool openInputStream(std::ifstream &out_stream) const;

public:
  RequestBody();
  ~RequestBody();

  size_t size() const { return total_size; };
  bool isFile() const { return is_file; }
  const std::string &getTempFilePath() const { return temp_file_path; }
  const unsigned char *getBuffer() const { return arena.getInternalBuffer(); }

  bool init(size_t size);
  bool append(const char *buf, size_t size);
  void finalize();

  ReadResult read();
  void resetReader();
};
