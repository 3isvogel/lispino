#ifndef MEM_H
#define MEM_H

extern const unsigned int HEAP_MAX_LEN;
extern const unsigned int TOKENBUF_MAX_LEN;
extern const unsigned int STACK_MAX_LEN;

int init_memory();
void del_memory();

void* halloc(unsigned int size);

#endif//MEM_H
