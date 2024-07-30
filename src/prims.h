#ifndef PRIMS_H
#define PRIMS_H
#include "types.h"
#include "eval.h"

#define PRIMITIVE_LIST  \
X(+,    f_add)          \
X(-,    f_sub)          \
X(*,    f_mul)          \
X(/,    f_div)          \
X(car,  ret_car)        \
X(cdr,  ret_cdr)        \
X(cons, const_cons)     \
X(reset, env_reset)     \
X(=,    atom_eq)        \
X(?,    atom_type)

#define X(a, b) PRIMITIVE_INDEX_##b,
enum Prims_indexes {
    PRIMITIVE_LIST
    PRIMITIVE_INDEX_MAX
};
#undef X

typedef struct {Cell name; Closure procedure;} Prim;
extern Prim prim_env[PRIMITIVE_INDEX_MAX];

#endif//PRIMS_H
