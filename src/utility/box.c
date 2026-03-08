#include "box.h"

#define X(x) #x,
char* printableTag[TAG_SIZE] = {
TAG_LIST
};
#undef X

void setValue(BoxRef box, Value value) {
    box->value = value;
}

void setTag(BoxRef box, Tag tag) {
    box->tag = tag;
}

Value getValue(BoxRef box) {
    return box->value;
}

Tag getTag(BoxRef box) {
    return box->tag;
}

Box setBox(Value value, Tag tag) {
    return (Box) {
        .value = value,
        .tag = tag,
    };
}

char* strTag(Tag tag) {
    // TODO: check boundaries?
    return printableTag[tag];
}
