#pragma once

#include "box.h"

#define RATIO_FREE 4

Box* createHeap(unsigned int size);
void destroyHeap();
Box* memRequest(unsigned int size);
