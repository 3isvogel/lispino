#ifndef HEAP_H
#define HEAP_H

#ifndef HEAP_MAX_LEN
#define HEAP_MAX_LEN (1<<12)
#endif

#include "types.h"

Cell get_mem(int size);
int max_heap_size();
int heap_avail();
void gc();

#endif//HEAP_H
