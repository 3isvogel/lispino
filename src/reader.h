#ifndef READER_H
#define READER_H

#include "types.h"
#include "mem.h"

extern char* token_buffer;
extern unsigned int token_buffer_len;

char* init_reader(unsigned int size);
void del_reader();

Box Read();

#endif//READER_H
