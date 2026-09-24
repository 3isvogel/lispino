#include <stdio.h>
#include <utility/box.h>
#include <utility/log.h>
#include <utility/signals.h>

#include "private.h"
#include "system/printer.h"
#include "stack.h"
#include <memory/mem.h>
#include <memory/heap.h>

#include <string.h>

Stack stacks[STACK_NUM] = {0};

void destroyStacks() {
    for (StackId i = 0; i < STACK_NUM; i ++) {
        if(stacks[i].data != NULL) {
            free(stacks[i].data);
        }
        stacks[i].data = NULL;
        stacks[i].head = 0;
        stacks[i].size = 0;
        stacks[i].base = 0;
    }
}

Cons* createStacks(unsigned int size) {
    // Check that no stacks[id] already exists, if it does delete it
    destroyStacks();
    // Allocate a continuous block of Cons, will be used to store the symbol name
    // and value in a single cons
    // car -> name, cdr -> value
    for (StackId i = 0; i < STACK_NUM; i++) {
        stacks[i].data = (Cons*) halloc(sizeof(Cons) * size);
        if (!stacks[i].data) return NULL;
        stacks[i].size = size;
    }
    return (Cons*)1;
}

// NOTE: Do not expose, expose "defineSymbol" instead
/**
 * @brief Push a symbol on top of the symbol stack
 *
 * @param name 
 * @param definition 
 */
void symbolPush(StackId id, Box name, Box definition) {
    if(stacks[id].head == stacks[id].size) {
        fail(SIGNAL_STACK_FULL);
    }
    stacks[id].data[stacks[id].head ++ ] = (Cons) {
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

void framePush(StackId id) {
    Box name = boxNil(),
        definition = setBox(stacks[id].base, TAG_INT);
    symbolPush(id, name, definition);
    stacks[id].base = stacks[id].head;
}

// Need it here
void frameRst(StackId id) {
    stacks[id].head = stacks[id].base;
}

void framePop(StackId id) {
    frameRst(id);
    // Prevents going backward in the stacks[id]
    if(stacks[id].base == 0) return;
    Cons cons = stacks[id].data[--stacks[id].head];
    // Actual value is in the CDR !!
    stacks[id].base = getValue(&(cons.cdr));
}

Frame frameCurrent(StackId id) {
    Frame frame = (Frame) {
        .start = &stacks[id].data[stacks[id].base],
        .end = &stacks[id].data[stacks[id].head],
    };
    return frame;
}

int frameOuter(StackId id, Frame* frame) {
    if (frame->start == stacks[id].data)
        return 0;
    // Modify frame 
    frame->end = frame->start - 1;
    unsigned int idxStart = getValue(&(frame->end->cdr));
    frame->start = &stacks[id].data[idxStart];
    return 1;
}

Box defineSymbol(StackId id, Box name, Box definition) {
    // TODO: consider if using the length value for string operations or just
    //       exploit the null-termination

    // TODO: consider removing this check
    if (getTag(&name) != TAG_SYMBOL) {
        fprintf(stderr, "; DEFINE_SYMBOL: [%s]", strTag(getTag(&name)));
        Print(name);
        return boxSignal(SIGNAL_WRONG_TYPE);
    }

    // If a string is saved as X.AAA.ssss... just move to the next Box pointer
    char* rawName = getRaw(name);

    for (unsigned int index = stacks[id].base; index < stacks[id].head; index ++) {

        // Extract raw string, no need to check the type of this as I made it
        // impossible before to add any non-string symbol, if all additions are
        // done through this function there will never be non-string symbols
        char* currentName = getRaw(stacks[id].data[index].car);

        if(strcmp(rawName, currentName) == 0) {
            // Definition lives in the system memory already, simply update it,
            // losing old reference
            stacks[id].data[index].cdr = definition;
            return definition;
        }
    }

    // The symbol does not exist in the current stacks[id] frame, add it
    symbolPush(id, name, definition);

    return definition;
}

Box getSymbol(StackId id, BoxRef nameRef) {

    // TODO: consider removing this check
    if (getTag(nameRef) != TAG_SYMBOL) {
        fprintf(stderr, "; GET_SYMBOL: [%s]", strTag(getTag(nameRef)));
        Print(follow(nameRef));
        return boxSignal(SIGNAL_WRONG_TYPE);
    }
    // If a string is saved as X.AAA.ssss... just move to the next Box pointer
    char* rawName = getRaw(*nameRef);

    // Search from the inner to the outer frame, until found or last frame is
    // reached
    Frame frame = frameCurrent(id);
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
    } while(frameOuter(id, &frame));

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
