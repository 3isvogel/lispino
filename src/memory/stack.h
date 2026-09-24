#pragma once

#include <utility/box.h>
#include <utility/signals.h>

// Expose stacks without exposing pointers, allows application to reference them
// by index, data structure will be saved on a stack vector, stack content is
// allocated once using malloc
#define STACK_LIST \
X(SYM) \
X(BIND)

typedef enum {
#define X(x) x##_STACK,
    STACK_LIST
    STACK_NUM
#undef X
} StackId;

// Accessory struct to keep the sate of current frame
typedef struct {
    Cons *start, *end;
} Frame;

/**
 * @brief Deallocate the stack if it exists
 */
void destroyStacks();

/**
 * @brief Allocate a new stack
 *
 * @param size 
 * @return A pointer to cons, indicating if the operation was successful or not
 */
Cons* createStacks(unsigned int size);

/**
 * @brief Create a new frame in the symbol stack
 */
void framePush(StackId stack);

/**
 * @brief Reset the last frame of the symbol stack
 *
 * This function should operate the same as a subsequent call of
 * framePop(); framePush(); but I might optimize it
 */
void frameRst(StackId stack);

/**
 * @brief Delete the last frame from the symbol stack
 */
void framePop(StackId stack);

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
Box defineSymbol(StackId stack, Box name, Box definition);

/**
 * @brief Returns the value of a symbol
 *
 * @return the value of the symbol
 */
Box getSymbol(StackId stack, BoxRef nameRef);

/**
 * @brief Initialize the environment with the primitives
 */
void GCinitializeEnv();
