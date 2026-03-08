#include "log.h"
#include <stdarg.h>
#include <stdio.h>

LogLevel treshold_log_level = LOG_LEVEL_ERROR;

#define X(x)    #x,
const char *printableLogLevel[LOG_LEVEL_MAX] = {
    LOG_LEVEL_LIST
};
#undef X

const char *log_level_color[] = {SET2E(BG(YELLOW_CODE), FG(BLACK_CODE)),
                                 SET_E(FG(BLUE_CODE)), SET_E(FG(GREEN_CODE)),
                                 SET2E(BLINKING_CODE, FG(YELLOW_CODE)),
                                 SET2E(BOLD_CODE, FG(RED_CODE))};

void logSetLevel(LogLevel logLevel) { treshold_log_level = logLevel; }

void logPrint(LogLevel logLevel, const char *fileName, int lineNumber,
              const char *format, ...) {

#ifndef BUILD_RELEASE
  if (logLevel < treshold_log_level)
    return;
#endif // BUILD_RELEASE
  // print header
#ifdef LOG_NO_SOURCE_POSITION
  fprintf(stderr, "%s%-5s" RESET SET_E(LIGHT_CODE) " " RESET,
          log_level_color[logLevel], printableLogLevel[logLevel]);
#else
  fprintf(stderr, "%s%-5s" RESET SET_E(LIGHT_CODE) " %16s:%-3d " RESET,
          log_level_color[logLevel], printableLogLevel[logLevel], fileName,
          lineNumber);
#endif
  va_list arg;
  va_start(arg, format);
  vfprintf(stderr, format, arg);
  va_end(arg);
  fprintf(stderr, "\n");
}
