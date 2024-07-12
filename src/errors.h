#ifndef ERRORS_H
#define ERRORS_H

typedef enum {
    EOF_REACHED,
    OUT_OF_MEMORY,
    UNBALANCED,
    UNTERMINATED_STR,
    
    ERR_MAX,
}Status;

void fail(int e);
void caccamerda();

#endif//ERRORS_H
