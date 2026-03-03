#include "box.h"
#include "log.h"
#include "stack.h"
#include "heap.h"
#include "prims.h"
#include "errors.h"
#include "signals.h"
#include <string.h>
#include <stdlib.h>

typedef struct {
    Cons* data;
    unsigned int head;
    unsigned int size;
    unsigned int base;
} Stack;
Stack stack = (Stack) {
    .data = NULL,
    .head = 0,
    .size = 0,
};

typedef struct {
    unsigned int start, end;
}Frame;

Cons* createStack(unsigned int size);

/**
 * @brief Deallocate the stack if it exists
 */
void destroyStack() {
    if(stack.data != NULL) {
        free(stack.data);
    }
    stack.data = NULL;
    stack.head = 0;
    stack.size = 0;
    stack.base = 0;
}

/**
 * @brief Allocate a new stack
 *
 * @param size 
 * @return A pointer to cons, indicating if the operation was successful or not
 */
Cons* createStack(unsigned int size) {
    // Check that no stack already exists, if it does delete it
    destroyStack();
    // Allocate a continuous block of Cons, will be used to store the symbol name
    // and value in a single cons
    // car -> name, cdr -> value
    stack.data = (Cons*) halloc(size * sizeof(Cons));
    return stack.data;
}

/**
 * @brief Push a symbol on top of the symbol stack
 *
 * @param symbol 
 */
void symbolPush(Cons* symbol) {
    if(stack.head == stack.size) {
        fail(SIGNAL_STACK_FULL);
    }
    stack.data[stack.head ++ ] = *symbol;
}

// This is necessary when implementing functions, so I don't have to retain the
// number of pushed symbols somewhere

/**
 * @brief Create a new frame in the symbol stack
 */
void framePush() {
    Cons cons = (Cons) {
        // FIXME: flag it in such a way that when searching for symbols I will
        //       not follow this;
        .car = box(0, TAG_NIL),
        // Actual value is in the CDR !!
        .cdr = box(stack.base, TAG_INT),
    };
    symbolPush(&cons);
    stack.base = stack.head;
}

/**
 * @brief Reset the last frame of the symbol stack
 *
 * This function should operate the same as a subsequent call of
 * framePop(); framePush(); but I might optimize it
 */
void frameRst() {
    stack.head = stack.base;
}

/**
 * @brief Delete the last frame from the symbol stack
 */
void framePop() {
    frameRst();
    // Prevents going backward in the stack
    if(stack.base == 0) return;
    Cons cons = stack.data[--stack.head];
    // Actual value is in the CDR !!
    stack.base = getValue(&(cons.cdr));
}

// TODO: Do I really need this

/**
 * @brief Return the indexes of the current frame
 *
 * @return 
 */
Frame frameCurrent() {
    return (Frame) {
        .start = stack.base,
        .end = stack.head,
    };
}

/**
 * @brief Modify in place the parameter frame if an outer frame exists
 *
 * An outer frame is simply the one before in the stack
 *
 * @param frame 
 * @return NULL if there is no outer frame
 */
int* frameOuter(Frame* frame) {
    if (frame->start == 0)
        return NULL;
    unsigned int tEnd   = stack.base - 1,
                 tStart = getValue(&(stack.data[tEnd].cdr));
    // Modify frame in place
    frame->start = tStart;
    frame->end = tEnd;
    return (int*)1;
}

Box define_sym(Cell name, Box def) {
    // TODO: search if definition already exists in current stack
    int found;
    Cell p;
    for(p = ptr.base, found = 0; p<ptr.stack && (!found); p++) {
        found = !strcmp(raw_adr(name), raw_adr(p->name));
    }
    if (found) {
        (--p)->def = def;
    } else {
        stack_push((Cell_t){.name = name, .def = def});
    }
    return def;
}

Box get_sym(Cell name) {
    Cell env = get_env();
    do {
        for(Cell sym = env->base; sym < env->stack; sym++) {
            if(!strcmp(raw_adr(sym->name), raw_adr(name))) {
                return sym->def;
            }
        }
    } while((env = outer_env()));
    return box(ERR, SYMBOL_NOT_DEFINED);
}

void* env_init() {
    logInfo("Initialize env");
    for(Prim *p = prim_env; p < &prim_env[PRIMITIVE_INDEX_MAX]; p++) {
        if(!define_sym(p->name, box(PRI, LONG(p->procedure))))
            return NULL;
    }
    return prim_env;
}
