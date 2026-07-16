#pragma once
#include "lib/arena.hpp"
#include <fstream>

class RequestBody {
private:
  ArenaBlock arena;
  std::ofstream writef;
  std::ifstream readf;
  std::string filepath;
  size_t total_size;
  size_t file_read_offset;
  bool initialized;
  bool is_file;
  bool mem_read_done;

  std::string to_string_patch(int n) const;
  bool init_writef();

  template <typename StreamType>
  bool openfile(StreamType &stream, std::ios_base::openmode mode) {
    if (stream.is_open())
      return true;

    if (!is_file)
      return false;

    stream.open(filepath.c_str(), mode | std::ios::binary);
    return stream.is_open();
  }

  template <typename StreamType> bool closefile(StreamType &file) {
    if (file.is_open())
      file.close();
    return !file.is_open();
  }

public:
  RequestBody();
  ~RequestBody();

  size_t size() const { return total_size; };
  bool isFile() const { return is_file; }
  const std::string &getFilePath() const { return filepath; }
  const unsigned char *getBuffer() const { return arena.getBuffer(); }
  bool fitsContentLength(size_t chunk_size, size_t cl) const {
    return total_size <= cl && chunk_size <= cl - total_size;
  }

  bool init(size_t size, bool useDisk);
  bool append(const char *buf, size_t size);
  void finalize();

  enum ReadStatus {
    READ_CHUNK, // Successfully read a chunk, call read() again
    READ_DONE,  // Reached end of file (EOF) cleanly
    READ_ERROR  // An internal file I/O error occurred
  };

  struct ReadResult {
    ReadStatus status;
    ArenaBlock *block;
  };

  ReadResult read();
  void resetReader();
};
