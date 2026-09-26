#pragma once
#include "stack.h"
#include <utility/box.h>
#include <stdlib.h>

typedef struct {
    Cons* data;
    unsigned int head;
    unsigned int size;
    unsigned int base;
} Stack;
extern Stack stack;

/**
 * @brief Return the indexes of the current frame
 *
 * @return 
 */
Frame frameCurrent();

/**
 * @brief Modify in place the parameter frame if an outer frame exists
 *
 * An outer frame is simply the one before in the stack
 *
 * @param frame 
 * @return NULL if there is no outer frame
 */
int frameOuter(Frame* frame);
