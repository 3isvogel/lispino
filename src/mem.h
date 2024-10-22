#ifndef MEM_H
#define MEM_H

extern unsigned int HEAP_MAX_LEN;
extern unsigned int TOKENBUF_MAX_LEN;
extern unsigned int STACK_MAX_LEN;

int init_memory();
void del_memory();

#endif//MEM_H
