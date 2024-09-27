// logging.c
#include "log.h"
#include <stdarg.h>

static log_lvl current_log_level = LOG_LEVEL_INFO;
static const char *level_strings[] = {"FATAL", "ERROR", "WARN",
                                      "INFO",  "DEBUG", "TRACE"};

void log_set_level(log_lvl level) { current_log_level = level; }

void log_message(log_lvl level, const char *file, int line, const char *format,
                 ...) {
  if (level > current_log_level) {
    return;
  }

  time_t now = 0;
  time(&now);
  char *date = ctime(&now);
  date[strlen(date) - 1] = '\0'; // Remove newline

  fprintf(stderr, "%s [%s] %s:%d: ", date, level_strings[level], file, line);

  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);

  fprintf(stderr, "\n");
  fflush(stderr);
}
