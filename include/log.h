// log.h
#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <string.h>
#include <time.h>

typedef enum {
  LOG_LEVEL_FATAL = 0,
  LOG_LEVEL_ERROR,
  LOG_LEVEL_WARN,
  LOG_LEVEL_INFO,
  LOG_LEVEL_DEBUG,
  LOG_LEVEL_TRACE
} log_lvl;

void log_set_level(log_lvl level);
void log_message(log_lvl level, const char *file, int line, const char *format,
                 ...);

#define LOG_FATAL(...)                                                         \
  log_message(LOG_LEVEL_FATAL, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...)                                                         \
  log_message(LOG_LEVEL_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...)                                                          \
  log_message(LOG_LEVEL_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...)                                                          \
  log_message(LOG_LEVEL_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_DEBUG(...)                                                         \
  log_message(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_TRACE(...)                                                         \
  log_message(LOG_LEVEL_TRACE, __FILE__, __LINE__, __VA_ARGS__)

#endif // LOG_H
