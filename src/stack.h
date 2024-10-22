#ifndef STACKS_H
#define STACKS_H

#include <types.h>
#include "mem.h"

Cell_t* init_stack(unsigned int size);
void del_stack();

void* env_init();

void* frame_new();
void* frame_del();
void frame_rst();
Cell get_env();
Cell outer_env();
Box define_sym(Cell name, Box def);
Box get_sym(Cell name);

#endif//STACKS_H
