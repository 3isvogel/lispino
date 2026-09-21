#pragma once
// C++ code guard
#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include "term_colors.h"

#define LOG_LEVEL_LIST  \
X(ALLOC,    Alloc,      TERM_CODE_SET(TERM_CODE_FG(TERM_COLOR_BLUE)))                   \
X(DEBUG,    Debug,      TERM_CODE_SET(TERM_CODE_FG(TERM_COLOR_CYAN)))                   \
X(INFO,     Info,       TERM_CODE_SET(TERM_CODE_FG(TERM_COLOR_GREEN)))                  \
X(WARN,     Warn,       TERM_CODE_SET(TERM_CODE_FG(TERM_COLOR_YELLOW)))                 \
X(ERROR,    Error,      TERM_CODE_SET2(TERM_CODE_BOLD, TERM_CODE_FG(TERM_COLOR_RED)))

#define X(x,y,z)    LOG_LEVEL_##x,
typedef enum {
    LOG_LEVEL_LIST
    LOG_LEVEL_MAX
} LogLevel;
#undef X

/**
 * @brief Set log level
 *
 * @param logLevel 
 */
void logSetLevel(LogLevel logLevel);

/**
 * @brief Get log level
 *
 * @return 
 */
LogLevel logGetLevel();

/**
 * @brief Conditional print, variadic
 *
 * @param logLevel log level
 * @param fileName file name
 * @param lineNumber line number
 * @param format format string
 */
void logPrintF(LogLevel logLevel, const char *fileName, int lineNumber,
              const char *format, ...);

/**
 * @brief Conditional print, va_list
 *
 * @param logLevel log level
 * @param fileName file name
 * @param lineNumber line number
 * @param format format string
 * @param args va_list of arguments
 */
void logPrintVa(LogLevel logLevel, const char *fileName, int lineNumber,
              const char *format, va_list args);

// #define logPrint(logLevel, format, ...) logPrintF(logLevel, __FILE__, __LINE__, format, __VA_ARGS__)

#ifndef BUILD_RELEASE

#define logAlloc( ...) logPrintF(LOG_LEVEL_ALLOC, __FILE__, __LINE__,  __VA_ARGS__)
#define logDebug( ...) logPrintF(LOG_LEVEL_DEBUG, __FILE__, __LINE__,  __VA_ARGS__)
#define logInfo( ...) logPrintF(LOG_LEVEL_INFO, __FILE__, __LINE__,  __VA_ARGS__)
#define logWarning( ...) logPrintF(LOG_LEVEL_WARN, __FILE__, __LINE__,  __VA_ARGS__)
#define logError( ...) logPrintF(LOG_LEVEL_ERROR, __FILE__, __LINE__,  __VA_ARGS__)

#else//BUILD_RELEASE

#define logAlloc( ...)
#define logDebug( ...)
#define logInfo( ...)
#define logWarning( ...)
#define logError( ...)

#ifdef  logError
#undef  logError
#endif
#define logError( ...) logPrint(LOG_LEVEL_ERROR, __FILE__, __LINE__,  __VA_ARGS__);

#endif // BUILD_RELEASE

#ifdef __cplusplus
}
#endif
