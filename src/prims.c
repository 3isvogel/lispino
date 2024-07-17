#include "prims.h"
#include "errors.h"
#include "log.h"
#include <string.h>

// Can assume that a is always a CONS
// Find a way to collapse these definitaions

enum Op_index {
    ADD,
    SUB,
    MUL
};

static Box num_op(Cell a, unsigned int op_id) {
    int f = 0;
    Box tot;
    switch(get_tag(a->car)) {
        case F64: f |= 1;
        case INT:
            tot = get_num_f(a->car);
            break;
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
            default:
                return box(ERR, WRONG_TYPE);
        }
    }
    if(!f) tot = box(INT, (long)tot);
    return tot;
}

Box f_add(Cell a) { return num_op(a, ADD); }
Box f_sub(Cell a) { return num_op(a, SUB); }
Box f_mul(Cell a) { return num_op(a, MUL); }

Box f_div(Cell a) {
    Box t; int f = 0;
    Box tot;
    switch(get_tag(a->car)) {
        case F64: f |= 1;
        case INT:
            tot = get_num_f(a->car);
            break;
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
            default:
                return box(ERR, WRONG_TYPE);
        }
    }
    if(!f && tot == (long)tot) return box(INT, (long)tot);
    return tot;
};

Box ret_car(Cell a) {
    return a->car;
};

Box ret_cdr(Cell a) {
    return a->cdr;
};

struct {char* name; Closure procedure;} prim_env[] = {
    {"+", f_add},
    {"-", f_sub},
    {"*", f_mul},
    {"/", f_div},
    {"car", ret_car},
    {"cdr", ret_cdr},
    {"", 0}
};

Box prim_solve(char* name) {
    int i = 0;
    while(strlen(prim_env[i].name)) {
        if(!strcmp(prim_env[i].name, name))
            return box(PRI, LONG(prim_env[i].procedure));
        i++;
    }
    return box(ERR, SYMBOL_NOT_DEFINED);
}
