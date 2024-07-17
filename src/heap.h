#ifndef HEAP_H
#define HEAP_H

#ifndef HEAP_MAX_LEN
#define HEAP_MAX_LEN (1<<12)
#endif

#define RATIO_FREE 4

#define GC(p,l) if (heap_avail() < HEAP_MAX_LEN * sizeof(Box) / RATIO_FREE) gc(p,l);
#define C_GC() if  (heap_avail() < HEAP_MAX_LEN * sizeof(Box) / RATIO_FREE) GC(NULL, 0);

#include "types.h"

void clean_heap_ref();
Cell get_mem(int size);
int max_heap_size();
int heap_avail();
void gc(Box* usr_root, int size);
void inspect_heap();

#endif//HEAP_H
