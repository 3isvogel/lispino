#ifndef READER_H
#define READER_H

#include <utility/box.h>
#include <memory/mem.h>

extern char* token_buffer;
extern unsigned int token_buffer_len;

char* createParser(unsigned int size);
void destroyParser();

Box Read();

#endif//READER_H
