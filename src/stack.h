#ifndef STACKS_H
#define STACKS_H

#include <types.h>

#ifndef STACK_MAX_LEN
#define STACK_MAX_LEN 4096
#endif

void* env_init();

void* frame_new();
void* frame_del();
void frame_rst();
Cell get_env();
Cell outer_env();
Box define_sym(char* name, Box def);
Box get_sym(char* name);

#endif//STACKS_H
