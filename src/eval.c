#include "eval.h"
#include "types.h"
#include "errors.h"
#include "log.h"
#include "stack.h"
#include "heap.h"
#include "string.h"

Box eval_ast(Box ast) {
    Cell pre, post, new_head;
    switch(get_tag(ast)) {
        case SYM:
            return get_sym((char*)get_val(ast));
            break;
        case CON:
            pre = CELL(ast);
            new_head = (Cell)get_mem(sizeof(Cell_t));
            post = new_head;
            post->car = Eval(pre->car);
            while(get_tag(pre->cdr) == CON) {
                post->cdr = box(CON, (long)get_mem(sizeof(Cell_t)));
                pre = CELL(pre->cdr);
                post = CELL(post->cdr);
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
            Box val = f(CELL(ast->cdr));
            return val;
    }
    return box(ERR, NOT_A_FUNCTION);
}

#include "printer.h"

Box Eval(Box ast) {
    Box first; Cell cc;
    char* sym;
    switch(get_tag(ast)) {
        case CON:
            cc = CELL(ast);
            if(get_tag((first = cc->car)) == SYM) {
                if(!strcmp((char*)get_val(first), "define")) {
                    if(get_tag(cc->cdr) != CON)
                        return box(ERR, WRONG_ARGS_NUMBER);
                    if(get_tag((cc = CELL(cc->cdr))->car) != SYM)
                        return box(ERR, WRONG_ARGUMENTS);
                    sym = (char*)get_val(cc->car);
                    if(get_tag(cc->cdr) != CON)
                        return box(ERR, WRONG_ARGS_NUMBER);
                    ast = Eval(CELL(cc->cdr)->car);
                    if(get_tag(ast) == ERR) return ast;
                    return define_sym(sym, ast);
                }
            }
            ast = eval_ast(ast);
            return apply(CELL(ast));
    }
    return eval_ast(ast);
}
