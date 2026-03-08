#pragma once

#include <utility/box.h>

/**
 * @brief Print the value of a box
 *
 * If the box is a signal it will be printed to stderr, otherwise on stdout
 *
 * @param box 
 */
void Print(Box box);
