#include "log.h"
#include <stdarg.h>
#include <stdio.h>

#define X(x,y,z)    #x,
const char *log_level_printable[LOG_LEVEL_MAX] = {
    LOG_LEVEL_LIST
};
#undef X

LogLevel treshold_log_level = LOG_LEVEL_MAX - 1;

#define X(x,y,z)    z,
const char *log_level_color[] = {
    LOG_LEVEL_LIST
};
#undef X

void logSetLevel(LogLevel logLevel) { treshold_log_level = logLevel; }

LogLevel logGetLevel() { return treshold_log_level; }

void logPrintF(LogLevel logLevel, const char *fileName, int lineNumber,
              const char *format, ...) {
    va_list args;
    va_start(args, format);
    logPrintVa(logLevel, fileName, lineNumber, format, args);
    va_end(args);
}

void logPrintVa(LogLevel logLevel, const char *fileName, int lineNumber,
              const char *format, va_list arg) {

    if (logLevel >= LOG_LEVEL_MAX) logLevel = LOG_LEVEL_ERROR;

#ifndef BUILD_RELEASE       // Not defined functions on release, print in any case
    if (logLevel < treshold_log_level) return;
#endif // BUILD_RELEASE

#ifndef LOG_DISABLE_COLOR
    // with colors
    const char* color_string = log_level_color[logLevel];
#endif
    const char* log_level    = log_level_printable[logLevel];

// Prefix
#ifndef LOG_NO_SOURCE_POSITION
#ifndef LOG_DISABLE_COLOR
    // Source, colors

    fprintf(stderr, "%s%-5s" TERM_CODE_RESET TERM_CODE_SET(TERM_CODE_LIGHT) " %s:%-4d " TERM_CODE_RESET,
        color_string, log_level, fileName, lineNumber);

#else //LOG_DISABLE_COLOR
    // Source, no colors

    fprintf(stderr, "%-5s" " %s:%-4d ",
        log_level, fileName, lineNumber);

#endif//LOG_DISABLE_COLOR
#else //LOG_NO_SOURCE_POSITION
#ifndef LOG_DISABLE_COLOR
    // No source, colors

    fprintf(stderr, "%s%-5s " TERM_CODE_RESET,
        color_string, log_level);

#else //LOG_DISABLE_COLOR
    // No source, no colors

    fprintf(stderr, "%-5s ",
        log_level);

#endif//LOG_DISABLE_COLOR
#endif//LOG_NO_SOURCE_POSITION

    vfprintf(stderr, format, arg);
    fprintf(stderr, "\n");
}
