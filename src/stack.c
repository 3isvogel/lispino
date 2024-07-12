#include "stack.h"
#include "heap.h"
#include <string.h>
#include "log.h"

Cell_t stack[STACK_MAX_LEN];
Cell_t ptr = {.base = 0, .stack = 0};
Cell_t env = {.base = 0, .stack = 0};

static inline int avail() {return STACK_MAX_LEN - ptr.stack; }

int h = 0;

static inline void d(char* f, Cell_t cell) {
    if(!h) {logDebug("%-10s %18s %18s %4s %4s %4s == Stack", "function", "cell_car", "cell_cdr", "bp", "sp", "free"); h=1;}
    logDebug("%-10s %18p %18p %4d %4d %4d", f, cell.car, cell.cdr, ptr.base, ptr.stack, STACK_MAX_LEN - ptr.stack);
}

void* stack_push(Cell_t cell) {
    if (!avail()) return NULL;
    stack[ptr.stack++] = cell;
    d((char*)__FUNCTION__, cell);
    return stack;
}

void* stack_pop(Cell cell) {
    if(!ptr.stack) return NULL;
    *cell = stack[--ptr.stack];
    d((char*)__FUNCTION__, *cell);
    return stack;
}

void* frame_new() {
    logDebug("frame_new");
    if (!stack_push(ptr)) return NULL;
    ptr.base = ptr.stack;
    return stack;
}

void* frame_del() {
    ptr.stack = ptr.base;
    logDebug("frame_del");
    if(!stack_pop(&ptr)) return NULL;
    if(!ptr.base) gc();
    return stack;
}

void frame_rst() {
    ptr.stack = ptr.base;
    logDebug("frame_rst");
}

void* solve_env() {
    return NULL;
}
