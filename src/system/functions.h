#pragma once

#include <utility/box.h>

/**
 * @brief Returns a function pointer matching the special form's name
 *
 * @param name 
 * @return Function pointer to the special form, NULL if no match was found
 */
Function matchSpecialForm(char* name);
