//
// Created by palulukan on 1/9/26.
//

#ifndef LOG_H_A2CACB47C4BB4FA08679BD0C52936078
#define LOG_H_A2CACB47C4BB4FA08679BD0C52936078

#include <cstdio>
#include <cinttypes>
#include <ctime>

#include <sys/time.h>

// NOLINTBEGIN
#define LOG(level, ...)                         \
do {                                            \
  struct timeval curTime = {};                  \
  gettimeofday(&curTime, nullptr);              \
  struct tm tm = {};                            \
  gmtime_r(&curTime.tv_sec, &tm);               \
  fprintf(stderr,                               \
    "%d-%02d-%02d %02d:%02d:%02d.%06lu [%s] ",  \
    tm.tm_year + 1900,                          \
    tm.tm_mon + 1,                              \
    tm.tm_mday,                                 \
    tm.tm_hour,                                 \
    tm.tm_min,                                  \
    tm.tm_sec,                                  \
    curTime.tv_usec,                            \
    level                                       \
  );                                            \
  fprintf(stderr, __VA_ARGS__);                 \
  fputs("\n", stderr);                          \
} while(0)                                      \
// NOLINTEND

// #define TRACE(...)  LOG("TRACE", __VA_ARGS__)
#define TRACE(...) ((void)0)

#define DEBUG(...)  LOG("DEBUG", __VA_ARGS__)
//#define DEBUG(...) ((void)0)

#define INFO(...)   LOG("INFO ", __VA_ARGS__)

#define WARN(...)   LOG("WARN ", __VA_ARGS__)

#define ERROR(...)  LOG("ERROR", __VA_ARGS__)

#endif //LOG_H_A2CACB47C4BB4FA08679BD0C52936078
