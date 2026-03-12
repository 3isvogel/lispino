#include "box.h"
#include <utility/signals.h>

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

Box getCar(BoxRef boxRef) {
    Tag tag = getTag(boxRef);
    // Propagate signal
    if (tag == TAG_SIGNAL) {
        return *boxRef;
    } else if (tag != TAG_CONS && tag != TAG_CLOSURE) {
        return boxSignal(SIGNAL_WRONG_ARGUMENTS);
    } return ((Cons*)getValue(boxRef))->car;
}

Box getCdr(BoxRef boxRef) {
    Tag tag = getTag(boxRef);
    if (tag == TAG_SIGNAL) {
        return *boxRef;
    } else if (tag != TAG_CONS && tag != TAG_CLOSURE) {
        return boxSignal(SIGNAL_WRONG_ARGUMENTS);
    } return ((Cons*)getValue(boxRef))->cdr;
}

void setCar(BoxRef boxRef, Box value) {
    if (getTag(boxRef) == TAG_CONS)
        ((Cons*)getValue(boxRef))->car = value;
}

void setCdr(BoxRef boxRef, Box value) {
    if (getTag(boxRef) == TAG_CONS)
        ((Cons*)getValue(boxRef))->cdr = value;
}
