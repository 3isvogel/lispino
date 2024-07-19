#ifndef ERRORS_H
#define ERRORS_H

#include "types.h"

#define ERROR_LIST      \
X(EOF_REACHED)          \
X(OUT_OF_MEMORY)        \
X(UNBALANCED)           \
X(UNTERMINATED_STR)     \
X(SYMBOL_NOT_DEFINED)   \
X(UNEXPECTED_BRANCH)    \
X(NOT_A_FUNCTION)       \
X(WRONG_ARGUMENTS)      \
X(WRONG_ARGS_NUMBER)    \
X(WRONG_TYPE)           \
X(DIV_ZERO)             \
X(PTR_MOVED)
    
#define X(x) x,
typedef enum {
ERROR_LIST
    ERR_MAX
}Status;
#undef X

extern char* ERS[ERR_MAX];

void fail(int e);
Box err(int e);
void caccamerda();

#endif//ERRORS_H
