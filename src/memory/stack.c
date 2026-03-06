#include <utility/box.h>
#include <utility/log.h>
#include "stack.h"
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

// NOTE: Do not expose, expose "defineSymbol" instead
/**
 * @brief Push a symbol on top of the symbol stack
 *
 * @param name 
 * @param definition 
 */
void symbolPush(Box* name, Box* definition) {
    if(stack.head == stack.size) {
        fail(SIGNAL_STACK_FULL);
    }
    stack.data[stack.head ++ ] = (Cons) {
        .car = *name,
        .cdr = *definition,
    };
}

// This is necessary when implementing functions, so I don't have to retain the
// number of pushed symbols somewhere

/**
 * @brief Create a new frame in the symbol stack
 */
void framePush() {
    Box name = box(0, TAG_NIL),
        definition = box(stack.base, TAG_INT);
    symbolPush(&name, &definition);
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
int frameOuter(Frame* frame) {
    if (frame->start == 0)
        return 0;
    unsigned int tEnd   = stack.base - 1,
                 tStart = getValue(&(stack.data[tEnd].cdr));
    // Modify frame in place
    frame->start = tStart;
    frame->end = tEnd;
    return 1;
}

/**
 * @brief Define symbol in the current stack frame
 *
 * If symbol does not exists in the current frame create it, otherwise update 
 * the definition
 *
 * @param name 
 * @param definition 
 * @return 
 */
Box* defineSymbol(Box* name, Box* definition) {
    // TODO: consider if using the length value for string operations or just
    //       exploit the null-termination

    // TODO: consider removing this check
    if (getTag(name) != TAG_RAW) {
        logError("Trying to dereference a non-raw");
        fail(SIGNAL_WRONG_TYPE);
    }

    // If a string is saved as X.AAA.ssss... just move to the next Box pointer
    char* rawName = (char*)(name + 1);

    for (unsigned int index = stack.base; index < stack.head; index ++) {

        // Extract raw string, no need to check the type of this as I made it
        // impossible before to add any non-string symbol, if all additions are
        // done through this function there will never be non-string symbols
        char* currentName = ((char*)getValue(&(stack.data[index].car)))
                            + sizeof(Box);

        if(strcmp(rawName, currentName) == 0) {
            // Definition lives in the system memory already, simply update it,
            // losing old reference
            stack.data[index].cdr = *definition;
            return definition;
        }
    }
    
    // The symbol does not exist in the current stack frame, add it
    symbolPush(name, definition);

    return definition;
}

/**
 * @brief Returns the value of a symbol
 *
 * @return the value of the symbol
 */
Box getSymbol(Box* name) {

    // TODO: consider removing this check
    if (getTag(name) != TAG_RAW) {
        logError("Trying to dereference a non-raw");
        return boxSignal(SIGNAL_WRONG_TYPE);
    }

    // If a string is saved as X.AAA.ssss... just move to the next Box pointer
    char* rawName = (char*)(name + 1);

    // Search from the inner to the outer frame, until found or last frame is
    // reached
    Frame frame = frameCurrent();
    do {        
        for (unsigned int index = frame.start; index < frame.end; index ++) {
    
            // Extract raw string, no need to check the type of this as I made it
            // impossible before to add any non-string symbol, if all additions are
            // done through this function there will never be non-string symbols
            char* currentName = ((char*)getValue(&(stack.data[index].car)))
                                + sizeof(Box);
    
            // Really slow symbol search
            if(strcmp(rawName, currentName) == 0) {
                return stack.data[index].cdr;
            }
        }
    } while(frameOuter(&frame));
    
    // The symbol is not defined
    return boxSignal(SIGNAL_SYMBOL_NOT_DEFINED);
}

void initializeEnv() {
    logInfo("Initializing environment");
    // for(/* All primitives in the environment*/) {
    //    Box primitive = box(TAG_PRIMITIVE, procedure)
    //    // defineSymbol(<name>, primitive)
    // }
}

void* env_init() {
    logInfo("Initialize env");
    for(Prim *p = prim_env; p < &prim_env[PRIMITIVE_INDEX_MAX]; p++) {
        if(!define_sym(p->name, box(PRI, LONG(p->procedure))))
            return NULL;
    }
    return prim_env;
}
