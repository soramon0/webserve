#pragma once

#include <cstdarg>
#include <cstdio>

#ifdef __GNUC__
// a, b should be numbers. exmaple ATTRIBUTE_FORMAT(1, 2):
// 1: The format string is the 1st argument
// 2: The variadic arguments start at the 2nd position
#define ATTRIBUTE_FORMAT(a, b) __attribute__((format(printf, a, b)))
#else
#define ATTRIBUTE_FORMAT(a, b)
#endif

class Logger {
public:
  enum Level { LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL };

  static void setLevel(Level lvl);

  static void debug(const char *fmt, ...) ATTRIBUTE_FORMAT(1, 2);
  static void info(const char *fmt, ...) ATTRIBUTE_FORMAT(1, 2);
  static void warn(const char *fmt, ...) ATTRIBUTE_FORMAT(1, 2);
  static void error(const char *fmt, ...) ATTRIBUTE_FORMAT(1, 2);
  static void fatal(const char *fmt, ...) ATTRIBUTE_FORMAT(1, 2);
  static void fatalWith(int exitCode, const char *fmt, ...)
      ATTRIBUTE_FORMAT(2, 3);

private:
  static Level minLevel;

  static void log(FILE *writer, Level lvl, const char *fmt, va_list args);
};
