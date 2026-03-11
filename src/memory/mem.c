/**
 * @file
 * @brief Manages system memory creation and deletion
 */

#include "mem.h"
#include "stack.h"
#include "heap.h"

#include <utility/log.h>
#include <utility/hash.h>

#include <system/parser.h>

#define DEFAULT_HEAP_MAX_LEN                1000
#define DEFAULT_POINTER_REGISTRY_MAX_LEN    128
#define DEFAULT_TOKEN_BUFFER_MAX_LEN        128
#define DEFAULT_STACK_MAX_LEN               128
#define DEFAULT_MIN_RAW_MAP_LEN             DEFAULT_STACK_MAX_LEN            

const unsigned int HEAP_MAX_LEN = DEFAULT_HEAP_MAX_LEN;
const unsigned int POINTER_REGISTRY_MAX_LEN = DEFAULT_POINTER_REGISTRY_MAX_LEN;
const unsigned int TOKEN_BUFFER_MAX_LEN = DEFAULT_TOKEN_BUFFER_MAX_LEN;
const unsigned int STACK_MAX_LEN = DEFAULT_STACK_MAX_LEN;
const unsigned int MIN_RAW_MAP_LEN = DEFAULT_MIN_RAW_MAP_LEN;

/**
 * @brief Create data structures
 *
 * @return 0 if fails, != 0 otherwise
 */
int createMemory() {
    logDebug("Creating memory...");
    randSeed();
    return createParser(TOKEN_BUFFER_MAX_LEN)
        && createStack(STACK_MAX_LEN)
        && createPointerRegistry(POINTER_REGISTRY_MAX_LEN)
        && createHeap(HEAP_MAX_LEN);
        
}

/**
 * @brief Destroy memory data structures
 */
void destroyMemory() {
    logAlloc("DELETING MEMORY...");
    destroyHeap();
    destroyPointerRegistry();
    destroyStack();
    destroyParser();
}

/**
 * @brief Hook alloc: calls malloc and logs
 */
void* halloc(unsigned int size) {
    void* ret = malloc(size);
    if(ret) {
        logAlloc("malloc: allocated %d bytes at %p", size, ret);
    } else {
        logError("malloc: fail");
    }
    return ret;
}
