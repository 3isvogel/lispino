#include "box.h"
#include <system/printer.h>
#include <utility/signals.h>
#include <stdio.h>

#define X(x) #x,
char* printableTag[TAG_SIZE] = {
TAG_LIST
};
#undef X

const Box nil = (Box){
    0,
    TAG_NIL
};

char* strTag(Tag tag) {
    if (tag >= TAG_SIZE) {
        logWarning("Unknown tag: 0x%x", tag);
        return printableTag[TAG_SIGNAL];
    }
    return printableTag[tag];
}

Box getCar(BoxRef boxRef) {
    const Tag tag = getTag(boxRef);
    // Propagate signal
    if (tag == TAG_SIGNAL)
        return *boxRef;
    if (tag != TAG_CONS && tag != TAG_CLOSURE)
        return boxSignal(SIGNAL_WRONG_ARGUMENTS);
    return ((Cons*)getValue(boxRef))->car;
}

Box getCdr(BoxRef boxRef) {
    const Tag tag = getTag(boxRef);
    if (tag == TAG_SIGNAL)
        return *boxRef;
    if (tag != TAG_CONS && tag != TAG_CLOSURE)
        return boxSignal(SIGNAL_WRONG_ARGUMENTS);
    return ((Cons*)getValue(boxRef))->cdr;
}

void setCar(BoxRef boxRef, Box value) {
    if (unlikely(getTag(boxRef) != TAG_CONS)) {
        logError("Cannot set car for non-cos boxes");
        fprintf(stderr, "Assigning value: ");
        Print(value);
        fprintf(stderr, "To: ");
        Print(*boxRef);
        fail(SIGNAL_UNKNOWN_FAILURE);
    }
    ((Cons*)getValue(boxRef))->car = value;
}

void setCdr(BoxRef boxRef, Box value) {
    if (unlikely(getTag(boxRef) != TAG_CONS)) {
        logError("Cannot set cdr for non-cos boxes");
        fprintf(stderr, "Assigning value: ");
        Print(value);
        fprintf(stderr, "To: ");
        Print(*boxRef);
        fail(SIGNAL_UNKNOWN_FAILURE);
    }
    ((Cons*)getValue(boxRef))->cdr = value;
}
