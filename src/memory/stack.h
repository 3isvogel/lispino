#pragma once

#include <utility/box.h>
#include <utility/signals.h>

// Accessory struct to keep the sate of current frame
typedef struct {
    Cons *start, *end;
} Frame;

/**
 * @brief Deallocate the stack if it exists
 */
void destroyStack();

/**
 * @brief Allocate a new stack
 *
 * @param size 
 * @return A pointer to cons, indicating if the operation was successful or not
 */
Cons* createStack(unsigned int size);

/**
 * @brief Create a new frame in the symbol stack
 */
void framePush();

/**
 * @brief Reset the last frame of the symbol stack
 *
 * This function should operate the same as a subsequent call of
 * framePop(); framePush(); but I might optimize it
 */
void frameRst();

/**
 * @brief Delete the last frame from the symbol stack
 */
void framePop();

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
Box defineSymbol(Box name, Box definition);

/**
 * @brief Returns the value of a symbol
 *
 * @return the value of the symbol
 */
Box getSymbol(BoxRef nameRef);

/**
 * @brief Initialize the environment with the primitives
 */
void GCinitializeEnv();

// TODO: Merge with pointerRegistry
typedef struct {
    Box* data;
    unsigned int head,  //  <- Ultimately don't like this naming convention, but
                 size;  //  <- I will keep it consistent across stacks
} BindStack;
extern BindStack bindStack;

// TODO: documentation

void destroyBindStack();

unsigned int createBindStack(unsigned int size);

// NOTE: there is no need to reference frames, user has to count push/pop
// TODO: how to integrate con pointerregistry? maybe both Push and Reserve?
BoxRef bindStackReserveN(unsigned int n);

#define bindStackReserve()\
    /* Reserve a single element */\
    bindStackReserveN(1)

// NOTE: differently from pointerRegistry stack I need either a peek function or
// a value-returing pop function
Box bindStackPopN(unsigned int n);

#define bindStacPop() \
    /* Pops a single element */\
    bindStackPopN(1)


