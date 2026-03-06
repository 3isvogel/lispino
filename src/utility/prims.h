/**
 * @file
 * @brief Define primitives
 */

#pragma once

#include "box.h"
#include <system/eval.h>

// Associative list name-primitives
#define PRIMITIVE_LIST  \
X(+,     f_add)         \
X(-,     f_sub)         \
X(*,     f_mul)         \
X(/,     f_div)         \
X(car,   primitiveCar)  \
X(cdr,   primitiveCdr)  \
X(cons,  const_cons)    \
X(reset, env_reset)     \
X(=,     atom_eq)       \
X(?,     atom_type)     \

// Enumerate primitives
#define X(name, function) PRIMITIVE_##function,
typedef enum {
    PRIMITIVE_LIST
    PRIMITIVE_SIZE
} __Primitive;
#undef X

// Define Closure type as a function pointer from Box* to Box
typedef Box (*Closure)(Box*);
// Define primitive initialization: associate name-procedure
typedef struct { char* name; Closure procedure; } PrimitiveInit;
// Associative vector name-procedure, only used to initialize
extern PrimitiveInit primitiveInit[PRIMITIVE_SIZE];
