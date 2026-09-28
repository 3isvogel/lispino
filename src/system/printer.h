#pragma once

#include <utility/box.h>
#include <stdio.h>

typedef enum {
    NON_READABLE,
    READABLE
} Readable;

/**
 * @brief Print the value of a box
 *
 * @param box 
 * @param readable print readable
 */
void innerPrint(Box box, Readable readable, FILE *file);

/**
 * @brief Print the value of a box
 *
 * If the box is a signal it will be printed to stderr, otherwise on stdout
 *
 * @param box 
 */
void Print(Box box, FILE* file);
