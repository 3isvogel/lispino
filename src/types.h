#ifndef TYPES_H
#define TYPES_H

#include <stdlib.h>

typedef double Box;

typedef union Cell_t {
    struct {
        Box car,
            cdr;
    };
    struct {
        union Cell_t *base,
                     *stack;
    };
    struct {
        char *name;
        Box def;
    };
} Cell_t;
typedef Cell_t* Cell;
typedef Box (*Closure)(Cell);

#define TAG_SHIFT 48
#define MASK_SHIFT 52

#define BOX_MASK ((long)0x7ff << MASK_SHIFT)
#define TAG_MASK ((long)0xf << TAG_SHIFT)
#define PAYLOAD_MASK (((long)1 << TAG_SHIFT) - 1)

#define TYPE_LIST \
X(NIL) \
X(INT) \
X(F64) \
X(SYM) \
X(CON) \
X(PRI) \
X(CLO) \
X(LAB) \
X(ERR) \
X(MOV) \
X(STR)

#define X(x) x,
enum DataTypes {
TYPE_LIST
TYPE_MAX };
#undef X

extern char* type_name[TYPE_MAX];

#define LONG(x) (* (long*) &x)
#define BOX(x) (* (Box*) &x)
#define CLOSURE(x) (* (Closure*) &x)
#define CELL(x) ((Cell)get_val(x))

long is_box(Box x);
long get_tag(Box x);
long get_val(Box x);
Box get_num_f(Box x);

static inline Box box(int tag, long value) {
    // just to be safe keep guard
    long t = BOX_MASK | ((long)(tag & 0xf) << TAG_SHIFT) | (value & PAYLOAD_MASK);
    return BOX(t);
}

#define nil box(NIL, 0)

#endif//TYPES_H
