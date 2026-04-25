#include "log.hpp"
#include <cstdlib>

static const char *get_lvl_str(Logger::Level level);

Logger::Level Logger::minLevel = Logger::LOG_INFO;

void Logger::setLevel(Level lvl) { minLevel = lvl; }

void Logger::log(FILE *writer, Level lvl, const char *fmt, va_list args) {
  if (lvl < minLevel)
    return;

  fprintf(writer, "[%s] ", get_lvl_str(lvl));
  vfprintf(writer, fmt, args);
  fprintf(writer, "\r\n");
  fflush(writer);
}

void Logger::debug(const char *format, ...) {
  va_list args;
  va_start(args, format);
  log(stdout, LOG_DEBUG, format, args);
  va_end(args);
}

void Logger::info(const char *format, ...) {
  va_list args;
  va_start(args, format);
  log(stdout, LOG_INFO, format, args);
  va_end(args);
}

void Logger::warn(const char *format, ...) {
  va_list args;
  va_start(args, format);
  log(stdout, LOG_WARN, format, args);
  va_end(args);
}

void Logger::error(const char *format, ...) {
  va_list args;
  va_start(args, format);
  log(stderr, LOG_ERROR, format, args);
  va_end(args);
}

void Logger::fatal(const char *format, ...) {
  va_list args;
  va_start(args, format);
  log(stderr, LOG_FATAL, format, args);
  va_end(args);
  exit(1);
}

void Logger::fatalWith(int exitCode, const char *format, ...) {
  va_list args;
  va_start(args, format);
  log(stderr, LOG_FATAL, format, args);
  va_end(args);
  exit(exitCode);
}

static const char *get_lvl_str(Logger::Level level) {
  switch (level) {
  case Logger::LOG_DEBUG:
    return "DEBUG";
  case Logger::LOG_INFO:
    return "INFO";
  case Logger::LOG_WARN:
    return "WARN";
  case Logger::LOG_ERROR:
    return "ERROR";
  case Logger::LOG_FATAL:
    return "FATAL";
  default:
    return "UNKNOWN";
  }
}
