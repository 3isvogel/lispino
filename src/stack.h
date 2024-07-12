#ifndef STACKS_H
#define STACKS_H

#include <types.h>

#ifndef STACK_MAX_LEN
#define STACK_MAX_LEN 4096
#endif

void* stack_push(Cell_t cell);
void* frame_new();
void* frame_del();
void frame_rst();

#endif//STACKS_H
