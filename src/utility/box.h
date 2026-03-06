/**
 * @file
 * @brief Hide implementation of a box, so changing implementation doesn't
 * disrupt the code
 */

#pragma once

// Using NaN boxing it's possible to fit almost anything into a double, I will
// use a more understandable representation and then try to use the more
// optimized one
// typedef double Box;

typedef long long int Value;

typedef struct {
    // Doesn't really matter which type this has, as long as it fits 64 bits
    Value value;
    // Doesn't really matter which type this has, used as a mask
    int tag;
} Box;

// A cons contains two boxes: a car and a cdr
typedef struct {
    Box car,
        cdr;
} Cons;

#define TAG_LIST \
X(NIL) \
X(INT) \
X(F64) \
X(SYMBOL) \
X(CONS) \
X(PRIMITIVE) \
X(CLOSURE) \
X(LABEL) \
X(MOVED) \
X(RAW) \
X(STRING) \
X(SIGNAL)

#define X(x) TAG_##x,
typedef enum {
    TAG_LIST
    TAG_SIZE
} Tag;
#undef X

Value getValue(Box* box);
Tag getTag(Box* box);
void setValue(Box* box, Value value);
void setTag(Box* box, Tag tag);
Box box(Value value, Tag tag);
