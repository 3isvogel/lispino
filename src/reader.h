#ifndef READER_H
#define READER_H

#include "types.h"

#ifndef TOKENBUF_MAX_LEN
#define TOKENBUF_MAX_LEN 32
#endif

extern char token_buffer[(TOKENBUF_MAX_LEN + 1)];
extern unsigned int token_buffer_len;

Box Read();

#endif//READER_H
