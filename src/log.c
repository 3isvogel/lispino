#include "log.h"
#include <stdarg.h>
#include <stdio.h>

log_level_t treshold_log_level = LOG_LEVEL_ERROR;
const char *log_level_flag[] = {
    "ALLOC", "DEBUG", "INFO", "WARN", "ERROR",
};

const char *log_level_color[] = {SET2E(BG(YELLOW_CODE), FG(BLACK_CODE)),
                                 SET_E(FG(BLUE_CODE)), SET_E(FG(GREEN_CODE)),
                                 SET2E(BLINKING_CODE, FG(YELLOW_CODE)),
                                 SET2E(BOLD_CODE, FG(RED_CODE))};

void logSetLevel(log_level_t log_level) { treshold_log_level = log_level; }

void logPrint(log_level_t log_level, const char *file_name, int line,
              const char *format, ...) {
#ifndef BUILD_RELEASE
  if (log_level < treshold_log_level)
    return;
#endif // BUILD_RELEASE
  // print header
#ifdef LOG_NO_SOURCE_POSITION
  fprintf(stderr, "%s%-5s" RESET SET_E(LIGHT_CODE) " " RESET,
          log_level_color[log_level], log_level_flag[log_level]);
#else
  fprintf(stderr, "%s%-5s" RESET SET_E(LIGHT_CODE) " %s:%-3d " RESET,
          log_level_color[log_level], log_level_flag[log_level], file_name,
          line);
#endif
  va_list arg;
  va_start(arg, format);
  vfprintf(stderr, format, arg);
  va_end(arg);
  fprintf(stderr, "\n");
}
