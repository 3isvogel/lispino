#include "log.h"
#include "utility/signals.h"
#include "prims.h"
#include <memory/stack.h>
#include <memory/heap.h>
#include <utility/box.h>
#include <string.h>

// Can assume that a is always a CONS
// Find a way to collapse these definitaions

enum Op_index {
    ADD,
    SUB,
    MUL
};

#define REC_OP(_op, _val)                   \
    _op(box(CON, (long)&(Cell_t) {          \
        .car = box(INT, _val),              \
        .cdr = box(CON, (long)&(Cell_t) {   \
            .car = a->car,                  \
            .cdr = nil })}))

static Box num_op(Box b, unsigned int op_id) {
    if(get_tag(b) != CON) return box(ERR, WRONG_ARGS_NUMBER);
    Cell a = CELL(b);
    int f = 0, cnt = 0;
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
        cnt++;
    }
    if(op_id == SUB && !cnt) tot = - tot;
    if(!f) tot = box(INT, (long)tot);
    return tot;
}

Box f_add(Box a) { return num_op(a, ADD); }
Box f_sub(Box a) { return num_op(a, SUB); }
Box f_mul(Box a) { return num_op(a, MUL); }

Box f_div(Box b) {
    if(get_tag(b) != CON) return box(ERR, WRONG_ARGS_NUMBER);
    Cell a = CELL(b);
    Box t; int f = 0;
    Box tot;
    switch(get_tag(a->car)) {
        case F64: f |= 1;
        case INT:
            tot = get_num_f(a->car);
            break;
        case ERR: return a->car;
        default: return box(ERR, WRONG_TYPE);
    }
    if(get_tag(a->cdr) != CON) return REC_OP(f_div, 1);
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
}

/**
 * @brief Treats box as a Cons reference and return its car
 *
 * Used by basically all primitives to extract arguments, prevents dereferencing
 * a value that is not a cons
 *
 * @param box 
 * @return 
 */
Box getConsCar(Box* box) {
    // Ensures that a function is called on a cons "(+ . (1 . nil))" and prevents
    // calling on other types "(f . 1)" argument is not a cons
    if (getTag(box) != TAG_CONS) {
        // If argument is not a cons do not dereference it and return an error
        return box(TAG_SIGNAL, SIGNAL_WRONG_TYPE);
    }
    // If the argument is indeed a cons, follow its car
    return ((Cons*)getValue(box))->car;
}

/**
 * @brief Returns the car of a list
 *
 * @param box 
 * @return the car of a list
 */
Box primitiveCar(Box* arg) {
    Box list = getConsCar(arg);
    switch (getTag(&list)) {
        // Bubble up signal
    case TAG_SIGNAL: return list;
    case TAG_CONS:
    case TAG_CLOSURE:
        // If the type is a closure or a cons return the car
        return ((Cons*)getValue(list))->car;
    default:
        // Otherwise signal error
        return box(TAG_SIG, SIGNAL_WRONG_TYPE);
    }
}

/**
 * @brief Returns the car of a list
 *
 * @param box 
 * @return the car of a list
 */
Box primitiveCdr(Box* arg) {
    Box list = getConsCar(arg);
    switch (getTag(list)) {
        // Bubble up signal
    case TAG_SIGNAL: return list;
    case TAG_CONS:
    case TAG_CLOSURE:
        // If the type is a closure or a cons return the car
        return ((Cons*)getValue(list))->cdr;
    default:
        // Otherwise signal error
        return box(TAG_SIG, SIGNAL_WRONG_TYPE);
    }
}

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

Box atom_eq(Box b) {
    if(get_tag(b) != CON) return box(ERR, WRONG_ARGS_NUMBER);
    Box l = CELL(b)->car; int tl = get_tag(l);
    if(get_tag(CELL(b)->cdr) != CON) return box(ERR, WRONG_ARGS_NUMBER);
    Box r = CELL(CELL(b)->cdr)->car; int tr = get_tag(r);
    if(tl == ERR) return l;
    if(tr == ERR) return r;
    if(!((num_tag(tl) && num_tag(tr)) || (tl == tr))) return nil;
    switch(tl) {
        case STR:
        case SYM:
        case LAB:
            return strcmp(raw_adr(CELL(l)), raw_adr(CELL(r))) ? nil : box(INT, 1);
        case F64:
            return l == ((Box)get_num_f(r)) ? box(INT, 1) : nil;
        case INT:
            switch(tr) {
                case F64:
                    return (Box)get_num_f(l) == r ? box(INT, 1) : nil;
                case INT:
                    return get_val(l) == get_val(r) ? box(INT, 1) : nil;
            }
    }
    return nil;
}

/**
 * @brief Return the primitive type as an integer 
 *
 * @param arg 
 * @return 
 */
Box primitiveGetType(Box* arg) {
    // Try to extract the argument
    Box follow = getConsCar(arg);

    // If some error occurred bubble up
    if (getTag(&follow) == TAG_SIGNAL) {
        return follow;
    }

    // Otherwise return the signal as an argument
    return box(TAG_INT, getTag(&follow));
}

Box atom_type(Box arg) {
    if (get_tag(arg) != CON)
        return box(ERR, WRONG_ARGUMENTS);
    return box(INT, get_tag(CELL(arg)->car));
}

// Initialize vector with "primitive_name" -> Box(*procedure)(Box*)
#define X(a,b) {(PrimitiveInit) #a, b}
Prim prim_env[PRIMITIVE_SIZE] = {
    PRIMITIVE_LIST
};
#undef X
