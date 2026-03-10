/**
 * @file
 * @brief Define primitives
 */

#pragma once

#include <utility/box.h>

// Define primitive initialization: associate name-procedure
typedef struct { char* name; Primitive procedure; } PrimitiveInit;
// Associative vector name-procedure, only used to initialize
extern PrimitiveInit primitiveInit[PRIMITIVE_SIZE];
