#pragma once
#include <cassert>
#include <fstream>
#include <string>

std::string make_temp_path(const char *prefix);

struct ScopedFile {
  std::string path;

  explicit ScopedFile(const std::string &p, const std::string &content)
      : path(make_temp_path(p.c_str())) {
    std::ofstream out(path.c_str());

    if (!out)
      throw std::runtime_error(path + ": File creation failed");

    out << content;
    out.flush();

    if (!out)
      throw std::runtime_error(path + ": Write failed");

    out.close();
  }
  ~ScopedFile() { std::remove(path.c_str()); }

private:
  ScopedFile(const ScopedFile &);
  ScopedFile &operator=(const ScopedFile &);
};
