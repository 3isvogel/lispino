#include "heap.h"
#include "errors.h"
#include "log.h"

int heap[2][HEAP_MAX_LEN];
int heap_idx;
int curr_heap = 0;

#define HEAP_FAIL_ON_OOM

void gc() {
    logDebug("Called GC");
    logWarning("Garbage collector not yet implemented");
};

int heap_avail() {
    return (HEAP_MAX_LEN - heap_idx) * sizeof(int);
}

Cell get_mem(int size) {
    if (size <= 0) return NULL;
    if (size > heap_avail())
        gc();
    if (size > heap_avail()) {
        #ifdef HEAP_FAIL_ON_OOM
            fail(OUT_OF_MEMORY);
        #else
            return NULL;
        #endif
    }

    Cell mem = (Cell)&(heap[curr_heap][heap_idx]);
    heap_idx += (size-1)/sizeof(int) + 1;
    return mem;
}


int max_heap_size() {return HEAP_MAX_LEN;}
