#include "types.h"
#include "log.h"

#define X(x) #x,
char* type_name[TYPE_MAX] = {
TYPE_LIST
};
#undef X

long is_box(Box x) {
    return !(~LONG(x) & BOX_MASK);
}

long get_tag(Box x) {
    if(!is_box(x)) return F64;
    return (LONG(x) & TAG_MASK) >> TAG_SHIFT;
}

#define SIGN_MASK ((long)0x000080 << (5*8))

long get_val(Box x) {
    if(get_tag(x) == INT) {
        return (LONG(x) & SIGN_MASK) ?
            LONG(x) | ((long)0xffff << TAG_SHIFT) :
            LONG(x) & PAYLOAD_MASK;
    }
    return LONG(x) & PAYLOAD_MASK;
}

Box get_num_f(Box x) {
    if(!is_box(x)) return x;
    return (Box)get_val(x);
}
