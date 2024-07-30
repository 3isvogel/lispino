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
            return get_sym(CELL(ast));
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
    Box defs, args, rets;
    switch(get_tag(ast->car)) {
        case PRI:
            f = (Closure)get_val(ast->car);
            Box val = f(ast->cdr);
            return val;
        case CLO:
            frame_new();
            defs = CELL(ast->car)->car;
            args = ast->cdr;
            while(get_tag(defs) == CON) {
                if (get_tag(args) == CON) {
                    define_sym(CELL(CELL(defs)->car), CELL(args)->car);
                    args = CELL(args)->cdr;
                } else { define_sym(CELL(CELL(defs)->car), nil); }
                defs = CELL(defs)->cdr;
            }
            rets = Eval(CELL(CELL(ast->car)->cdr)->car);
            frame_del();
            return rets;
    }
    return box(ERR, NOT_A_FUNCTION);
}

Box lambda_ops(Box ast) {
    if(get_tag(ast) != CON)
        return box(ERR, WRONG_ARGS_NUMBER);
    int atag = get_tag(CELL(ast)->car);
    if(atag != NIL) {
        if(atag != CON)
            return box(ERR, LAMBDA_ARGS);
        Box a = CELL(ast)->car;
        while(get_tag(a) == CON) {
            if(get_tag(CELL(a)->car) != SYM)
                return box(ERR, LAMBDA_ARGS);
            a = CELL(a)->cdr;
        }
    }
    return box(CLO, get_val(ast));
}

#include "printer.h"
#include <stdio.h>

Box def_ops(Box ast) {
    Cell cc, tmp, sym;
    if(get_tag(ast) != CON)
        return box(ERR, WRONG_ARGS_NUMBER);
    switch(get_tag((cc = CELL(ast))->car)) {
        case SYM:
            sym = CELL(cc->car);
            if(get_tag(cc->cdr) != CON)
                return box(ERR, WRONG_ARGS_NUMBER);
            ast = Eval(CELL(cc->cdr)->car);
            if(get_tag(ast) == ERR) return ast;
            break;
        case CON:
            sym = CELL(CELL(cc->car)->car);
            tmp = (Cell)get_mem(sizeof(Cell_t));
            tmp->car = CELL(cc->car)->cdr;
            tmp->cdr = cc->cdr;
            ast = lambda_ops(box(CON, LONG(tmp)));
            break;
        default:
            return box(ERR, WRONG_ARGUMENTS);
    }
    return define_sym(sym, ast);
}

#include "printer.h"

Box Eval(Box ast) {
    Box first; Cell cc;
    char* sym;
    switch(get_tag(ast)) {
        case CON:
            cc = CELL(ast);
            if(get_tag((first = cc->car)) == SYM) {
                if(!strcmp((sym = raw_adr(CELL(first))), "define")) {
                    return def_ops(cc->cdr);
                } else if(!strcmp(sym, "lambda")) {
                    return lambda_ops(cc->cdr);
                } else if(!strcmp(sym, "quote")) {
                    if(get_tag(cc->cdr) != CON)
                        return box(ERR, WRONG_ARGUMENTS);
                    return CELL(cc->cdr)->car;
                } else if(!strcmp(sym, "if")) {
                    if(get_tag(cc->cdr) != CON || get_tag((cc = CELL(cc->cdr))->cdr) != CON)
                        return box(ERR, WRONG_ARGS_NUMBER);
                    if(get_tag(Eval(cc->car)) != NIL)
                        return Eval((cc = CELL(cc->cdr))->car);
                    if(get_tag(cc->cdr) == CON) {
                        cc = CELL(cc->cdr);
                        if(get_tag(cc->cdr) == CON)
                            return Eval(CELL(cc->cdr)->car);
                        return box(NIL, 0);
                    } return box(ERR, WRONG_ARGUMENTS);
                } else if (!strcmp(sym, "do")) {
                    first = nil;
                    ast = cc->cdr;
                    while(get_tag(ast) == CON) {
                        cc = CELL(ast);
                        first = Eval(cc->car);
                        ast = cc->cdr;
                    };
                    return first;
                }
            }
            ast = eval_ast(ast);
            return apply(CELL(ast));
    }
    return eval_ast(ast);
}
