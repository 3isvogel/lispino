#include "eval.h"
#include "types.h"
#include "errors.h"
#include "prims.h"
#include "log.h"
#include "heap.h"

Box sym_solve(char* name) {
    // local env resolve
    // global env resolve
    return prim_solve(name);
}

Box eval_ast(Box ast) {
    Cell pre, post, new_head;
    switch(get_tag(ast)) {
        case SYM:
            return sym_solve((char*)get_val(ast));
            break;
        case CON:
            pre = (Cell)get_val(ast);
            new_head = (Cell)get_mem(sizeof(Cell_t));
            post = new_head;
            post->car = Eval(pre->car);
            while(get_tag(pre->cdr) == CON) {
                post->cdr = box(CON, (long)get_mem(sizeof(Cell_t)));
                pre = (Cell)get_val(pre->cdr);
                post = (Cell)get_val(post->cdr);
                post->car = Eval(pre->car);
            }
            post->cdr = Eval(pre->cdr);
            return box(CON, LONG(new_head));
            break;
    }
    return ast;
}

Box apply(Cell ast) {
    Closure f;
    switch(get_tag(ast->car)) {
        case PRI:
            // Check only on primitives, allows creating clojures with no args
            if(get_tag(ast->cdr) != CON) return box(ERR, WRONG_ARGS_NUMBER);
            f = (Closure)get_val(ast->car);
            Box val = f((Cell)get_val(ast->cdr));
            return val;
    }
    return box(ERR, NOT_A_FUNCTION);
}

#include "printer.h"

Box Eval(Box ast) {
    switch(get_tag(ast)) {
        case CON:
            ast = eval_ast(ast);
            return apply((Cell)get_val(ast));
    }
    return eval_ast(ast);
}
