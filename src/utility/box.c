#include "box.h"

#define X(x) #x,
char* printableTag[TAG_SIZE] = {
TAG_LIST
};
#undef X

void setValue(Box* box, Value value) {
    box->value = value;
}

void setTag(Box* box, Tag tag) {
    box->tag = tag;
}

Value getValue(Box* box) {
    return box->value;
}

Tag getTag(Box* box) {
    return box->tag;
}

Box box(Value value, Tag tag) {
    return (Box) {
        .value = value,
        .tag = tag,
    };
}
