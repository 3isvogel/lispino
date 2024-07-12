#include "types.h"
#include "log.h"

Box nil;

void type_init() {
    nil = box(NIL, 0);
}

long is_box(Box x) {
    return !(~LONG(x) & BOX_MASK);
}

long get_tag(Box x) {
    if(!is_box(x)) return DOUB;
    return (LONG(x) & TAG_MASK) >> TAG_SHIFT;
}

long get_val(Box x) {
    return LONG(x) & PAYLOAD_MASK;
}

Box box(int tag, long value) {
    // just to be safe keep guard
    long t = BOX_MASK | ((long)(tag & 0xf) << TAG_SHIFT) | (value & PAYLOAD_MASK);
    return BOX(t);
}
