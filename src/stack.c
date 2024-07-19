#include "log.h"
#include "stack.h"
#include "heap.h"
#include "prims.h"
#include "errors.h"
#include <string.h>

Cell_t stack[STACK_MAX_LEN];
Cell_t ptr = {.base = stack, .stack = stack};

static inline int avail() {return &stack[STACK_MAX_LEN] - ptr.stack; }

static inline void d(char* f, Cell_t cell) {
    static int h = 0;
    if(!h) {logDebug("%-10s %18s %18s %12s %12s %4s == Stack", "function", "cell_car", "cell_cdr", "bp", "sp", "free"); h=1;}
    logDebug("%-10s %18p %18p %12x %12x %4d", f, cell.car, cell.cdr, ptr.base, ptr.stack, avail());
}

void* stack_push(Cell_t cell) {
    if (!avail()) fail(OUT_OF_STACK);//return NULL;
    *(ptr.stack) = cell;
    ptr.stack ++;
    d((char*)__FUNCTION__, cell);
    return stack;
}

void* stack_pop(Cell cell) {
    if(ptr.stack == stack) return NULL;
    ptr.stack--;
    *cell = *(ptr.stack);
    d((char*)__FUNCTION__, *cell);
    return stack;
}

void* frame_new() {
    logDebug("frame_new");
    stack_push(ptr);
    ptr.base = ptr.stack;
    return stack;
}

void* frame_del() {
    ptr.stack = ptr.base;
    logDebug("frame_del");
    if(!stack_pop(&ptr)) return NULL;
    // if(ptr.base == stack) gc(NULL,0);
    return stack;
}

void frame_rst() {
    ptr.stack = ptr.base;
    logDebug("frame_rst");
}

Cell_t stack_env = {.base = 0, .stack = 0};

Cell get_env() {
    stack_env.base  = ptr.base;
    stack_env.stack = ptr.stack;
    return &stack_env;
}

Cell outer_env() {
    if(stack_env.base == stack) return NULL;
    stack_env = *(--stack_env.base);
    return &stack_env;
}

#include "printer.h"

Box define_sym(char* name, Box def) {
    // TODO: search if definition already exists in current stack
    int found;
    Cell p;
    for(p = ptr.base, found = 0; p<ptr.stack && (!found); p++) {
        found = !strcmp(name, p->name);
    }
    if (found) {
        (--p)->def = def;
    } else {
        stack_push((Cell_t){.name = name, .def = def});
    }
    return def;
}

Box get_sym(char* name) {
    Cell env = get_env();
    do {
        for(Cell sym = env->base; sym < env->stack; sym++) {
            if(!strcmp(sym->name, name)) {
                return sym->def;
            }
        }
    } while((env = outer_env()));
    return box(ERR, SYMBOL_NOT_DEFINED);
}

void* env_init() {
    logInfo("Initialize env");
    for(Prim *p = prim_env; p < &prim_env[PRIMITIVE_INDEX_MAX]; p++) {
        if(!define_sym(p->name, box(PRI, LONG(p->procedure))))
            return NULL;
    }
    return prim_env;
}
