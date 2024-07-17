#ifndef STACKS_H
#define STACKS_H

#include <types.h>

#ifndef STACK_MAX_LEN
#define STACK_MAX_LEN 4096
#endif

// TODO: remove
void* stack_push(Cell_t cell);
void* frame_new();
void* frame_del();
void frame_rst();
Cell get_env();
Cell outer_env();
Box define_sym(char* name, Box def);

#endif//STACKS_H
