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

#define NIL  0
#define INT  1
#define DOUB 2
#define SYM  3
#define CONS 4
#define PRIM 5
#define CLOS 6
#define LABL 7
#define STRI 8

#define LONG(x) (* (long*) &x)
#define BOX(x) (* (Box*) &x)

extern Box nil;

void type_init();
long is_box(Box x);
long get_tag(Box x);
long get_val(Box x);
Box box(int tag, long value);

#endif//TYPES_H
