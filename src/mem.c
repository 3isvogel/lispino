/**
 * @file
 * @brief Manages system memory creation and deletion
 */

#include "mem.h"
#include "parser.h"
#include "stack.h"
#include "heap.h"
#include "log.h"

#include <stdlib.h>

#define DEFAULT_HEAP_MAX_LEN      2048
#define DEFAULT_TOKENBUF_MAX_LEN  2048
#define DEFAULT_STACK_MAX_LEN     512

const unsigned int HEAP_MAX_LEN = DEFAULT_HEAP_MAX_LEN;
const unsigned int TOKENBUF_MAX_LEN = DEFAULT_TOKENBUF_MAX_LEN;
const unsigned int STACK_MAX_LEN = DEFAULT_STACK_MAX_LEN;

int init_memory() {
    logDebug("Creating memory...");
    return createHeap(HEAP_MAX_LEN)
        && init_stack(STACK_MAX_LEN)
        && createParser(TOKENBUF_MAX_LEN);
}

void del_memory() {
    destroyHeap();
    del_stack();
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
