#include "box.h"
#include <utility/signals.h>

#define X(x) #x,
char* printableTag[TAG_SIZE] = {
TAG_LIST
};
#undef X

const Box nil = (Box){
    0,
    TAG_NIL
};

void setValue(BoxRef boxRef, Value value) {
    boxRef->value = value;
}

void setTag(BoxRef boxRef, Tag tag) {
    boxRef->tag = tag;
}

Value getValue(BoxRef boxRef) {
    return boxRef->value;
}

Tag getTag(BoxRef boxRef) {
    return boxRef->tag;
}

Box setBox(Value value, Tag tag) {
    return (Box) {
        .value = value,
        .tag = tag,
    };
}

char* strTag(Tag tag) {
    if (tag >= TAG_SIZE)
        return printableTag[TAG_SIGNAL];
    return printableTag[tag];
}

Box getCar(BoxRef boxRef) {
    Tag tag = getTag(boxRef);
    // Propagate signal
    if (tag == TAG_SIGNAL)
        return *boxRef;
    if (tag != TAG_CONS && tag != TAG_CLOSURE)
        return boxSignal(SIGNAL_WRONG_ARGUMENTS);
    return ((Cons*)getValue(boxRef))->car;
}

Box getCdr(BoxRef boxRef) {
    Tag tag = getTag(boxRef);
    if (tag == TAG_SIGNAL)
        return *boxRef;
    if (tag != TAG_CONS && tag != TAG_CLOSURE)
        return boxSignal(SIGNAL_WRONG_ARGUMENTS);
    return ((Cons*)getValue(boxRef))->cdr;
}

void setCar(BoxRef boxRef, Box value) {
    if (getTag(boxRef) == TAG_CONS)
        ((Cons*)getValue(boxRef))->car = value;
}

void setCdr(BoxRef boxRef, Box value) {
    if (getTag(boxRef) == TAG_CONS)
        ((Cons*)getValue(boxRef))->cdr = value;
}
