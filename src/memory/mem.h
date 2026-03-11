#pragma once

#include <utility/box.h>

extern const unsigned int HEAP_MAX_LEN;
extern const unsigned int TOKEN_BUFFER_MAX_LEN;
extern const unsigned int STACK_MAX_LEN;
extern const unsigned int POINTER_REGISTRY_MAX_LEN;
extern const unsigned int MIN_RAW_MAP_LEN;

int createMemory();
void destroyMemory();

void* halloc(unsigned int size);
