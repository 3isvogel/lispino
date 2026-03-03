#pragma once

#include "box.h"

extern const unsigned int HEAP_MAX_LEN;
extern const unsigned int TOKENBUF_MAX_LEN;
extern const unsigned int STACK_MAX_LEN;

int init_memory();
void del_memory();

Box* halloc(unsigned int size);
