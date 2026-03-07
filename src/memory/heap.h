/**
 * @file
 * @brief 
 */

#pragma once

#include <utility/box.h>

/**
 * @brief return the amount of available bytes in the heap
 */
unsigned int heapAvailableSize();

/**
 * @brief Follow a box reference if it's valid, returns a signal otherwise
 *
 * @param boxRef 
 * @return 
 */
Box follow(BoxRef boxRef);

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
 * @brief Calls GC - and requests a specified amount of bytes
 *
 * @param size 
 * @return The address of the allocated memory
 */
BoxRef newMem(unsigned int size);

/**
 * @brief Calls GC - and requests enough memory for a Cons
 *
 * @return Thea ddress of the allocated memory
 */
Cons* newCons();

/**
 * @brief Calls GC - and requests enough memory for a raw string
 *
 * Use setRaw(BoxRef, char* , len) to make sure the value is set properly
 *
 * @param len 
 * @return 
 */
BoxRef newRaw(unsigned int len);

/**
 * @brief Sets the value of a raw string
 *
 * @param len 
 * @return a nil box on success, a signaled box otherwise
 */
Box setRaw(BoxRef boxRef, char *string);

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
void pointerRegistryPush(BoxRef boxRef);

/**
 * @brief pop the last pointer in the registry
 */
void pointerRegistryPop();
