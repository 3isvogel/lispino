#include "heap.h"
#include "stack.h"
#include "errors.h"
#include "log.h"
#include "string.h"
#include <stdlib.h>

Box** heap; // [2][HEAP_MAX_LEN];
int heap_idx = 0;
int curr_heap = 0;

Cell stack_base = NULL;

#define HEAP_FAIL_ON_OOM

int init_heap(unsigned int size) {
    int success = 1;
    heap = (Box**)malloc(sizeof(Box*) * 2);
    success = success && heap;
    for(int i=0; i<2 && success; i++) {
        heap[i] = (Box*)malloc(sizeof(Box) * size);
        success = success && heap[i];
    }
    logAlloc("Allocating heap at %p (%p, %p)", heap, heap[0], heap[1]);
    return success;
}
void del_heap(){
    for(int i=0; i<2; i++)
        free(heap[i]);
    free(heap);
}

void find_stack_base() {
    stack_base = get_env();
    logDebug("Stack base: %p", stack_base);
    Cell t;
    while((t = outer_env())) {
        stack_base = t;
    }
    stack_base = stack_base->base;
}

Cell name_move(Cell name) {
    int len = strlen(raw_adr(name));
    Cell new_name = rawcpy(raw_mem(len + 1), name, len + 1);
    *((Box*)name) = box(MOV, LONG(new_name));
    return new_name;
}

Cell cons_move(Cell cons) {
    Cell new_cons = (Cell)memcpy(get_mem(sizeof(Cell_t)), cons, sizeof(Cell_t));
    *((Box*)cons) = box(MOV, LONG(new_cons));
    return new_cons;
}

Box* check_moved(Box old) {
    Box mov = *((Box*)get_val(old));
    if(get_tag(mov) == MOV)
        return (Box*)get_val(mov);
    return NULL;
}

Box box_move(Box node) {
    Cell new_node;
    Box *moved;
    switch(get_tag(node)) {
        case CON:
        case CLO:
            // it is a s-expr:
            if((moved = check_moved(node)))
                return box(get_tag(node), LONG(moved));
            new_node = cons_move((Cell)get_val(node));
            new_node->car = box_move(new_node->car);
            new_node->cdr = box_move(new_node->cdr);
            return box(get_tag(node), LONG(new_node));
        case STR:
        case SYM:
        case LAB:
            /// Please, don't use this for exploits, I don't want to implement
            /// additional metadata
            /// :|
            /// FIXME: for now just always clone all strings,
            /// ideal is to mark moved strings
            /// - either prevent any \x3F\xFX character from being written to 
            /// prevent misinterpretation of data (it would allow user to
            /// specify an address for the moved struct, breaking everything)
            /// - or just add metadata at the beginning of the string:
            /// 2 bytes containing \x3F\xFx, specifying if string is valid (RAW)
            /// or is moved (MOV), in the first case treat the string normally
            /// in the second case treat first 8 bytes as a moved value (2 bytes
            /// TAG and 48 bytes address of moved value)
            /// ----
            if((moved = check_moved(node)))
                return box(get_tag(node), LONG(moved));
            return box(get_tag(node), (long)name_move(CELL(node)));
    }
    return node;
}

void* gc(Box* usr_root, int size) {
    logDebug("Called GC");
    if(!stack_base) find_stack_base();
    Cell stack_top = get_env()->stack;
    heap_idx = 0;
    curr_heap ^= 1;
    for(Cell root = stack_base; root<stack_top; root++) {
        if((Box*)root->name >= &heap[0][0] && (Box*)root->name < &heap[1][HEAP_MAX_LEN]) {
            root->name = name_move(root->name);
            root->def = box_move(root->def);
        }
    }
    for(int i = 0; i < size; i++) {//update
        usr_root[i] = box_move(usr_root[i]);
    }
    return NULL;
}

int heap_avail() {
    return (HEAP_MAX_LEN - heap_idx) * sizeof(Box);
}

Cell get_mem(int size) {
    logAlloc("Requesting: %dB", size);
    if (size <= 0) return NULL;
    if (size > heap_avail()) {
        #ifdef HEAP_FAIL_ON_OOM
            fail(OUT_OF_MEMORY);
        #else
            return NULL;
        #endif
    }

    Cell mem = (Cell)&(heap[curr_heap][heap_idx]);
    heap_idx += (size-1)/sizeof(Box) + 1;
    logAlloc("Providing:  %dB", (char*)&heap[curr_heap][heap_idx] - (char*)mem);
    return mem;
}

int max_heap_size() {return HEAP_MAX_LEN;}

Cell rawcpy(Cell dst, Cell src, int len) {
    if(len < 0) return dst;
    *((Box*)dst) = box(RAW, 0);
    memcpy(raw_adr(dst), raw_adr(src), len);
    return dst;
}

Cell raw_mem(int len) {
    if(len < 0) return NULL;
    Cell ret = get_mem(len + 2);
    *((Box*)ret) = box(RAW, 0);
    return ret;
}
