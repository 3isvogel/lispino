#ifndef HEAP_H
#define HEAP_H

#include "types.h"
#include "mem.h"

#define RATIO_FREE 4

#define GC(p,l) ((heap_avail() < HEAP_MAX_LEN * sizeof(Box) / RATIO_FREE) ? gc(p,l) : NULL)
#define C_GC()  GC(NULL, 0)

int init_heap(unsigned int size);
void del_heap();

static inline char* raw_adr(Cell adr) {
    return ((char*)adr) + 2;
}

#include "types.h"

void clean_heap_ref();
Cell get_mem(int size);
int max_heap_size();
int heap_avail();
void* gc(Box* usr_root, int size);
void inspect_heap();
Cell rawcpy(Cell dst, Cell src, int len);
Cell raw_mem(int len);

#endif//HEAP_H
