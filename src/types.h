#ifndef TYPES_H
#define TYPES_H

#include <stdlib.h>

typedef double Box;

typedef union {
    struct {
        Box car,
            cdr;
    };
    struct {
        int base,
            stack;
    };
} Cell_t;
typedef Cell_t* Cell;

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
X(STR)

#define X(x) x,
enum DataTypes {
TYPE_LIST
TYPE_MAX };
#undef X

extern char* type_name[TYPE_MAX];

#define LONG(x) (* (long*) &x)
#define BOX(x) (* (Box*) &x)

extern Box nil;

void type_init();
long is_box(Box x);
long get_tag(Box x);
long get_val(Box x);
Box box(int tag, long value);

#endif//TYPES_H
