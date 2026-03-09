/**
 * @file
 * @brief Contains signals definition, to make different parts of the program
 *        communicate
 */

#pragma once

#include "box.h"
#include "log.h"
#include <memory/mem.h>
#include <stdlib.h>

// All signals hava a name and a description, which is printed on fail

#define SIGNAL_LIST         \
X(UNKNOWN_FAILURE, "")      \
X(MEM_SETUP_FAIL, "")       \
X(EOF_REACHED, "")          \
X(OUT_OF_MEMORY, "")        \
X(UNTERMINATED_STR, "")     \
X(SYMBOL_NOT_DEFINED, "")   \
X(UNEXPECTED_BRANCH, "")    \
X(NOT_A_FUNCTION, "")       \
X(WRONG_ARGUMENTS, "")      \
X(WRONG_ARGS_NUMBER, "")    \
X(WRONG_TYPE, "")           \
X(DIV_ZERO, "")             \
X(ENV_INIT_FAIL, "")        \
X(OUT_OF_STACK, "")         \
X(LAMBDA_ARGS, "")          \
X(PTR_MOVED, "")            \
/* Introducing new error types, should use these instead to fail */ \
X(TOKEN_TOO_LONG, "")       \
X(HEAP_FULL, "")            \
X(STACK_FULL, "")           \
X(POINTER_REGISTRY_FULL, "")\
X(SYNTAX_ERROR, "")         \
X(BAD_REFERENCE, "")        \
X(FAIL_RAWMEMORY_CHECK, "") \
X(POINTER_REGISTRY_LEAKING, "")\
X(POINTER_REGISTRY_EMPTY, "")\
X(TO_DO, "")
    
#define X(x,s) SIGNAL_##x,
typedef enum {
SIGNAL_LIST
    SIGNAL_SIZE
}Signal;
#undef X

/**
 * @brief Prints message and fail
 *
 * This needs to be a macro to make sure the line and file reported are correct
 * ones
 *
 * @param message 
 */
#define todo(message) do { logError("TODO: %s", message); destroyMemory(); exit(SIGNAL_TO_DO); } while (0)


/**
 * @brief Prints the readable signal and ends the process cleaning its memory
 *
 * @param signal
 */
void _lineFail(Signal signal, char* file, int line);

// Make sure to propagate line and file
#define fail(signal) _lineFail(signal, __FILE__, __LINE__)

/**
 * @brief Incapsulates signal into a Box
 *
 * @param signal
 * @return boxed signal
 */
Box boxSignal(Signal signal);

/**
 * @brief Returns the printable verion of a signal
 *
 * @param signal 
 * @return 
 */
char* strSignal(Signal signal);

/**
 * @brief Copies a boxed signal to another box
 *
 * @param dest 
 * @param src 
 * @return a positive number if dest was overwritten, 0 otherwise
 */
unsigned int signalPass(BoxRef dest, BoxRef src);
