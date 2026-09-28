#include <stdio.h>
#include <utility/box.h>
#include <utility/log.h>
#include <utility/signals.h>

#include "private.h"
#include "system/functions.h"
#include "system/printer.h"
#include "stack.h"
#include <memory/mem.h>
#include <memory/heap.h>

#include <string.h>

Stack stack = (Stack) {
    .data = NULL,
    .head = 0,
    .size = 0,
};

void destroyStack() {
    if(stack.data != NULL) {
        free(stack.data);
    }
    stack.data = NULL;
    stack.head = 0;
    stack.size = 0;
    stack.base = 0;
}

Cons* createStack(unsigned int size) {
    // Check that no stack already exists, if it does delete it
    destroyStack();
    // Allocate a continuous block of Cons, will be used to store the symbol name
    // and value in a single cons
    // car -> name, cdr -> value
    stack.data = (Cons*) halloc(sizeof(Cons) * size);
    stack.size = size;
    return stack.data;
}

// NOTE: Do not expose, expose "defineSymbol" instead
/**
 * @brief Push a symbol on top of the symbol stack
 *
 * @param name 
 * @param definition 
 */
void symbolPush(Box name, Box definition) {
    if(stack.head == stack.size) {
        fail(SIGNAL_STACK_FULL);
    }
    stack.data[stack.head ++ ] = (Cons) {
        .car = name,
        .cdr = definition,
    };
}

// This is necessary when implementing functions, so I don't have to retain the
// number of pushed symbols somewhere

// TODO: ok, linear search is slow, consider using hashmap:
// +-------+
// |       |    +--+    +--+    +--+
// | "car" | -> |  | -> |  | -> |  | -> ;
// |       |    +- +    +--+    +--+
// +-------+
// |       | ; List of all occurrencies of "car" symbol, every new definition
// |       | ; pushes on head, when frame is deleted, pop from head (new def)
// |       | ; Hopefully this would reflect the state of the stack inside an
// +-------+ ; hashmap
// |       |
// |       |
// |       |
// +-------+
// |       |
// |       |
// |       |
// +-------+

void framePush() {
    Box name = boxNil(),
        definition = setBox(stack.base, TAG_INT);
    symbolPush(name, definition);
    stack.base = stack.head;
}

// Need it here
void frameRst() {
    stack.head = stack.base;
}

void framePop() {
    frameRst();
    // Prevents going backward in the stack
    if(stack.base == 0) return;
    Cons cons = stack.data[--stack.head];
    // Actual value is in the CDR !!
    stack.base = getValue(&(cons.cdr));
}

Frame frameCurrent() {
    Frame frame = (Frame) {
        .start = &stack.data[stack.base],
        .end = &stack.data[stack.head],
    };
    return frame;
}

int frameOuter(Frame* frame) {
    if (frame->start == stack.data)
        return 0;
    // Modify frame 
    frame->end = frame->start - 1;
    unsigned int idxStart = getValue(&(frame->end->cdr));
    frame->start = &stack.data[idxStart];
    return 1;
}

Box defineSymbol(Box name, Box definition) {
    // TODO: consider if using the length value for string operations or just
    //       exploit the null-termination

    // TODO: consider removing this check
    if (getTag(&name) != TAG_SYMBOL) {
        fprintf(stderr, "; DEFINE_SYMBOL: [%s]", strTag(getTag(&name)));
        Print(name, stderr);
        return boxSignal(SIGNAL_WRONG_TYPE);
    }

    // If a string is saved as X.AAA.ssss... just move to the next Box pointer
    char* rawName = getRaw(name);

    for (unsigned int index = stack.base; index < stack.head; index ++) {

        // Extract raw string, no need to check the type of this as I made it
        // impossible before to add any non-string symbol, if all additions are
        // done through this function there will never be non-string symbols
        char* currentName = getRaw(stack.data[index].car);

        if(strcmp(rawName, currentName) == 0) {
            // Definition lives in the system memory already, simply update it,
            // losing old reference
            stack.data[index].cdr = definition;
            return definition;
        }
    }
    
    // The symbol does not exist in the current stack frame, add it
    symbolPush(name, definition);

    return definition;
}

Box getSymbol(BoxRef nameRef) {

    // TODO: consider removing this check
    if (getTag(nameRef) != TAG_SYMBOL) {
        fprintf(stderr, "; GET_SYMBOL: [%s]", strTag(getTag(nameRef)));
        Print(follow(nameRef), stderr);
        return boxSignal(SIGNAL_WRONG_TYPE);
    }
    // If a string is saved as X.AAA.ssss... just move to the next Box pointer
    char* rawName = getRaw(*nameRef);

    // Search from the inner to the outer frame, until found or last frame is
    // reached
    Frame frame = frameCurrent();
    do {
        for (Cons* cons = frame.start; cons < frame.end; cons ++) {

            // Extract raw string, no need to check the type of this as I made it
            // impossible before to add any non-string symbol, if all additions are
            // done through this function there will never be non-string symbols
            char* currentName = getRaw(cons->car);

            // Really slow symbol search
            if(strcmp(rawName, currentName) == 0) {
                return cons->cdr;
            }
        }
    } while(frameOuter(&frame));

    // The symbol is not defined
    logError("; Symbol not defined: %s", rawName);
    return boxSignal(SIGNAL_SYMBOL_NOT_DEFINED);
}

// void* env_init() {
//     logInfo("Initialize env");
//     for(Prim *p = prim_env; p < &prim_env[PRIMITIVE_INDEX_MAX]; p++) {
//         if(!define_sym(p->name, box(PRI, LONG(p->procedure))))
//             return NULL;
//     }
//     return prim_env;
// }

// TODO: This is a copy-paste of pointer registry to handle different types, they will eventually
//       collide into a single implementation: either by becoming generic stacks
//       or by using pointerRegistry to store boxes rather than pointers (works the other way around)

BindStack bindStack = {0};

void destroyBindStack() {
    if (bindStack.data != NULL) free(bindStack.data);
    bindStack.data = NULL;
    bindStack.head = 0;
}

unsigned int createBindStack(unsigned int size) {
    destroyBindStack();

    // NOTE: It would take just a small change to merge the two implementations
    bindStack.data = (Box*) halloc(size * sizeof(Box));
    if(!bindStack.data) return  0;
    bindStack.size = size;
    bindStack.head = 0;
    return 1;
}

// NOTE: there is no need to reference frames, user has to count push/pop
BoxRef bindStackReserveN(unsigned int n) {
    if (bindStack.head + n > bindStack.size) fail(SIGNAL_BINDING_STACK_FULL);
    const BoxRef ret = &bindStack.data[bindStack.head];
    bindStack.head += n;
    return ret;
}

// NOTE: differently from pointerRegistry stack I need either a peek function or
// a value-returing pop function
Box bindStackPopN(unsigned int n) {
    if (unlikely(bindStack.head < n)) fail(SIGNAL_BINDING_STACK_EMPTY);
    const Box ret = bindStack.data[bindStack.head];
    bindStack.head-=n;
    return ret;
}
