/**
 * @file
 * @brief 
 */

#pragma once

#include <utility/box.h>

/**
 * @brief Deallocate the heap if it exists
 */
void destroyHeap();

/**
 * @brief Allocate a new heap
 *
 * @param size 
 * @return A pointer to cons, indicating if the operation was successful or not
 */
Box* createHeap(unsigned int size);

/**
 * @brief Calls garbage collector and requests a specified amount of bytes
 *
 * @param size 
 * @return The address of the allocated memory
 */
Box* memRequest(unsigned int size);

/**
 * @brief Destroy pointer registry if it exitss
 */
void destroyPointerRegistry();

/**
 * @brief Allocate a new pointer regitsry
 *
 * @param size 
 */
unsigned int createPointerRegistry(unsigned int size);

/**
 * @brief Register a box pointer to the registry
 *
 * @param boxPtr
 */
void pointerRegistryPush(Box** boxPtr);

/**
 * @brief pop the last pointer in the registry
 */
void pointerRegistryPop();
