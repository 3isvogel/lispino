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
void initializeEnv();
