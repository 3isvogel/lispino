#pragma once

#include <utility/box.h>

typedef enum
{
    FORM_COMPOSITE = 0,
    FORM_LEAF = 1,
} FormType;

typedef enum
{
    ADD = 0, SUB, MUL, DIV
} Arithmetic;

/**
 * @brief Returns a primitive from a box
 *
 * @param box 
 * @return 
 */
Function getPrimitive(Box box);

/**
 *
 * @brief Returns a function pointer matching the special form's name
 *
 * @param name 
 * @return Function pointer to the special form, NULL if no match was found
 */
SpecialForm matchSpecialForm(Box box, FormType* isLeafStatement);
