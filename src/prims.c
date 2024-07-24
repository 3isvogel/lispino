#include "heap.h"
#include "errors.h"
#include "log.h"
#include "prims.h"
#include "stack.h"
#include <string.h>

// Can assume that a is always a CONS
// Find a way to collapse these definitaions

enum Op_index {
    ADD,
    SUB,
    MUL
};

static Box num_op(Box b, unsigned int op_id) {
    Cell a = CELL(b);
    int f = 0;
    Box tot;
    switch(get_tag(a->car)) {
        case F64: f |= 1;
        case INT:
            tot = get_num_f(a->car);
            break;
        case ERR: return a->car;
        default: return box(ERR, WRONG_TYPE);
    }
    while(get_tag(a->cdr) == CON) {
        a = (Cell)get_val(a->cdr);
        switch(get_tag(a->car)) {
            case F64: f |= 1;
            case INT:
                switch(op_id) {
                case ADD:
                    tot += get_num_f(a->car);
                    break;
                case SUB:
                    tot -= get_num_f(a->car);
                    break;
                case MUL:
                    tot *= get_num_f(a->car);
                    break;
                }
                break;
            case ERR: return a->car;
            default: return box(ERR, WRONG_TYPE);
        }
    }
    if(!f) tot = box(INT, (long)tot);
    return tot;
}

Box f_add(Box a) { return num_op(a, ADD); }
Box f_sub(Box a) { return num_op(a, SUB); }
Box f_mul(Box a) { return num_op(a, MUL); }

Box f_div(Box b) {
    Box t; int f = 0;
    Box tot;
    Cell a = CELL(b);
    switch(get_tag(a->car)) {
        case F64: f |= 1;
        case INT:
            tot = get_num_f(a->car);
            break;
        case ERR: return a->car;
        default: return box(ERR, WRONG_TYPE);
    }
    while(get_tag(a->cdr) == CON) {
        a = (Cell)get_val(a->cdr);
        switch(get_tag(a->car)) {
            case F64: f |= 1;
            case INT:
                t = get_num_f(a->car);
                if(t == 0) {
                    return box(ERR, DIV_ZERO);
                }
                tot /= t;
                break;
            case ERR: return a->car;
            default: return box(ERR, WRONG_TYPE);
        }
    }
    if(!f && tot == (long)tot) return box(INT, (long)tot);
    return tot;
};

#include "printer.h"

Box ret_car(Box b) {
    Cell a = CELL(b);
    switch(get_tag(a->car)) {
        case ERR: return a->car;
        case CON:
        case CLO: 
            return (*(Cell)get_val(a->car)).car;
    }
    return box(ERR, WRONG_TYPE);
};

Box ret_cdr(Box b) {
    Cell a = CELL(b);
    switch(get_tag(a->car)) {
        case ERR: return a->car;
        case CON:
        case CLO: 
            return (*(Cell)get_val(a->car)).cdr;
    }
    return box(ERR, WRONG_TYPE);
};

Box const_cons(Box b) {
    if(get_tag(b) != CON) return box(ERR, WRONG_ARGS_NUMBER);
    Cell a = CELL(b);
    Box car = a->car, cdr = a->cdr;
    Cell ccar = CELL(car), ccdr = CELL(cdr);
    if(get_tag(a->car) == ERR) return a->car;
    switch(get_tag(cdr)) {
        //case ERR: return cdr;
        case CON:
            if(get_tag(ccdr->car) == ERR) return ccdr->car;
            Cell mem = get_mem(sizeof(Cell_t));
            mem->car = a->car;
            mem->cdr = ccdr->car;
            return box(CON, LONG(mem));
    }
    return box(ERR, WRONG_ARGS_NUMBER);
}

Box env_reset(Box a) {
    while(frame_del());
    gc(NULL,0);
    env_init();
    return box(NIL, 0);
}

#define X(a,b) {(Cell)"\xff\xff" #a, b},
Prim prim_env[PRIMITIVE_INDEX_MAX] = {
    PRIMITIVE_LIST
};
#undef X
