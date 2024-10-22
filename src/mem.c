#include "mem.h"
#include "heap.h"
#include "reader.h"
#include "stack.h"
#include "log.h"

#define DEFAULT_HEAP_MAX_LEN      2048
#define DEFAULT_TOKENBUF_MAX_LEN  2048
#define DEFAULT_STACK_MAX_LEN     512

unsigned int HEAP_MAX_LEN = DEFAULT_HEAP_MAX_LEN;
unsigned int TOKENBUF_MAX_LEN = DEFAULT_TOKENBUF_MAX_LEN;
unsigned int STACK_MAX_LEN = DEFAULT_STACK_MAX_LEN;

int init_memory() {
    logDebug("Creating memory...");
    return init_heap(HEAP_MAX_LEN)
        && init_stack(STACK_MAX_LEN)
        && init_reader(TOKENBUF_MAX_LEN);
}

void del_memory() {
    del_heap();
    del_stack();
    del_reader();
}
