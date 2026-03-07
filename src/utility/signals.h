/**
 * @file
 * @brief Contains signals definition, to make different parts of the program
 *        communicate
 */

#pragma once

#include "box.h"

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
X(TOKEN_BUFFER_FULL, "")    \
X(HEAP_FULL, "")            \
X(STACK_FULL, "")           \
X(POINTER_REGISTRY_FULL, "")\
X(SYNTAX_ERROR, "")         \
X(BAD_REFERENCE, "")        \
X(FAIL_RAWMEMORY_CHECK, "") \
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
 * @param message 
 */
void todo(const char* const message);

/**
 * @brief Prints the readable signal and ends the process cleaning its memory
 *
 * @param signal
 */
void fail(Signal signal);

/**
 * @brief Incapsulates signal into a Box
 *
 * @param signal
 * @return boxed signal
 */
Box boxSignal(Signal signal);

/**
 * @brief returns the printable verion of a signal
 *
 * @param signal 
 * @return 
 */
char* strSignal(Signal signal);
