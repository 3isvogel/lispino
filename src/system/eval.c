#include "eval.h"
#include <utility/box.h>
#include <utility/signals.h>
#include <memory/stack.h>
#include <memory/heap.h>

#include <string.h>

// Box eval_ast(Box ast) {
//     Cell pre, post, new_head;
//     switch(get_tag(ast)) {
//         case SYM:
//             return get_sym(CELL(ast));
//             break;
//         case CON:
//             pre = CELL(ast);
//             new_head = (Cell)get_mem(sizeof(Cell_t));
//             post = new_head;
//             post->car = Eval(pre->car);
//             while(get_tag(pre->cdr) == CON) {
//                 post->cdr = box(CON, (long)get_mem(sizeof(Cell_t)));
//                 pre = CELL(pre->cdr);
//                 post = CELL(post->cdr);
//                 post->car = Eval(pre->car);
//             }
//             post->cdr = Eval(pre->cdr);
//             return box(CON, LONG(new_head));
//             break;
//     }
//     return ast;
// }
// 
// Box lambda_ops(Box ast) {
//     if(get_tag(ast) != CON)
//         return box(ERR, WRONG_ARGS_NUMBER);
//     int atag = get_tag(CELL(ast)->car);
//     if(atag != NIL) {
//         if(atag != CON)
//             return box(ERR, LAMBDA_ARGS);
//         Box a = CELL(ast)->car;
//         while(get_tag(a) == CON) {
//             if(get_tag(CELL(a)->car) != SYM)
//                 return box(ERR, LAMBDA_ARGS);
//             a = CELL(a)->cdr;
//         }
//     }
//     return box(CLO, get_val(ast));
// }
// 
// #include <stdio.h>
// 
// Box def_ops(Box ast) {
//     Cell cc, tmp, sym;
//     if(get_tag(ast) != CON)
//         return box(ERR, WRONG_ARGS_NUMBER);
//     switch(get_tag((cc = CELL(ast))->car)) {
//         case SYM:
//             sym = CELL(cc->car);
//             if(get_tag(cc->cdr) != CON)
//                 return box(ERR, WRONG_ARGS_NUMBER);
//             ast = Eval(CELL(cc->cdr)->car);
//             if(get_tag(ast) == ERR) return ast;
//             break;
//         case CON:
//             sym = CELL(CELL(cc->car)->car);
//             tmp = (Cell)get_mem(sizeof(Cell_t));
//             tmp->car = CELL(cc->car)->cdr;
//             tmp->cdr = cc->cdr;
//             ast = lambda_ops(box(CON, LONG(tmp)));
//             break;
//         default:
//             return box(ERR, WRONG_ARGUMENTS);
//     }
//     return define_sym(sym, ast);
// }
// 
// int frame_lvl = 0;
//

// Given a list of evaluated elements: apply the first argument (function) to the
// rest of the arguments
Box applyList(BoxRef boxRef) {

    // functionBox contains the function to apply
    Box functionBox = ((Cons*)getValue(boxRef))->car;
    // boxRef contains instead
        *boxRef     = ((Cons*)getValue(boxRef))->cdr;

    Tag tag = getTag(&functionBox);
    if (tag != TAG_PRIMITIVE && tag != TAG_CLOSURE) return boxSignal(SIGNAL_NOT_A_FUNCTION);

    return boxNil();
}

void evalForm(BoxRef boxRef);

void evalList(BoxRef argsBoxRef) {
    // If argument is not a Cons (e.g (+ . 3) return signal
    if (getTag(argsBoxRef) != TAG_CONS) {
        *argsBoxRef = boxSignal(SIGNAL_WRONG_TYPE);
        return;
    }

    // TODO: as in parser: this may be implemented as a first part "evalArgs"
    //       and a second "evalArgsRecursive" which exploits tail call recursion
    //       to have a neat and efficient recursive implementattion with less
    //       repeated code
    Box evaluatedHead = boxNil(),
        evaluatedTail = boxNil(),
        valueBox = boxNil();

    // 3 Pointers to construct a list (2 + temporary value)
    pointerRegistryPush(&evaluatedHead);
    pointerRegistryPush(&evaluatedTail);
    pointerRegistryPush(&valueBox);

    // Set new cons as head
    Cons* consRef = newCons();
    evaluatedHead = setBox((Value) consRef, TAG_CONS);
    evaluatedTail = evaluatedHead;

    // Set value of first cons
    valueBox = ((Cons*)getValue(argsBoxRef))->car;
    evalForm(&valueBox);
    // End on signal
    if (getTag(&valueBox) == TAG_SIGNAL) {
        evaluatedHead = valueBox;
        goto evalArgsEnd;
    }

    // Add the value to result list and continue
    ((Cons*)getValue(&evaluatedTail))->car = valueBox;
    Tag cdrTag = getTag(&((Cons*)getValue(argsBoxRef))->cdr);

    // Until list end
    while (cdrTag == TAG_CONS) {
        
        // Drop reference to previous elements of the list (forward scan)
        *argsBoxRef = ((Cons*)getValue(argsBoxRef))->cdr;

        // Add cons, link it and set its car as "valueBox"
        consRef = newCons();
        ((Cons*)getValue(&evaluatedTail))->cdr = setBox((Value) consRef, TAG_CONS);
        evaluatedTail = setBox((Value) consRef, TAG_CONS);

        // Set value of current cons
        valueBox = ((Cons*)getValue(argsBoxRef))->car;
        evalForm(&valueBox);
        // End on signal
        if (getTag(&valueBox) == TAG_SIGNAL) {
            evaluatedHead = valueBox;
            goto evalArgsEnd;
        }

        ((Cons*)getValue(&evaluatedTail))->car = valueBox;
        cdrTag = getTag(&((Cons*)getValue(argsBoxRef))->cdr);
    
    }
    ((Cons*)getValue(argsBoxRef))->cdr = boxNil();

evalArgsEnd:
    pointerRegistryPop();
    pointerRegistryPop();
    pointerRegistryPop();
   
    *argsBoxRef = evaluatedHead;
}

void evalForm(BoxRef boxRef) {

    // Temporary tag value
    Tag tag = getTag(boxRef);

    // Symbols are solved from env
    if (tag == TAG_SYMBOL) {
        *boxRef = getSymbol(boxRef);
    // Cons are evaluated as:
    //  If firs element is a symbol solve it and use it as a function / closure
    //  use other elements as parameters
    } else if (tag == TAG_CONS) {
        Box functionBox = ((Cons*)getValue(boxRef))->car,
            argumentBox = ((Cons*)getValue(boxRef))->cdr;

        // First element is a symbol: check if it mathces special forms
        if (getTag(&functionBox) == TAG_SYMBOL) {
            char* name = getRaw(functionBox);
            // quote is pretty easy to evaluate: just return the argument
            // without evaluation
            if (strcmp("quote", name) == 0) {
                if (getTag(&argumentBox) != TAG_CONS) {
                    *boxRef = boxSignal(SIGNAL_WRONG_ARGS_NUMBER);
                    return;
                }
                *boxRef = ((Cons*)getValue(&argumentBox))->car;
                return;
            }
            // First element is a symbol, but is not a special form: evaluate
            // all elements of the list, invalidates functionBox and argumentBox
            // but they are no longer needed: next step is to apply function to
            // arguments
            //
            // boxRef is now referencing a list of evaluated values
            evalList(boxRef);
            if (getTag(boxRef) == TAG_SIGNAL) return;
            *boxRef = applyList(boxRef);
            if (getTag(boxRef) == TAG_SIGNAL) return;
            
        }
        todo("Implement eval");
    }
}

Box Eval(Box box) {

    // Register this box before doing additional processing
    pointerRegistryPush(&box);

    evalForm(&box);

    pointerRegistryPop();

    return box;
}

// Box Eval(Box ast) {
//     Box first; Cell cc;
//     char* sym;
//     // apply
//     Closure f;
//     Box defs, args, rets; Cell cast;
// restart:
//     logDebug("Evaling: %lx", LONG(ast));
//     switch(get_tag(ast)) {
//         case CON:
//             cc = CELL(ast);
//             if(get_tag((first = cc->car)) == SYM) {
//                 if(!strcmp((sym = raw_adr(CELL(first))), "define")) {
//                     return def_ops(cc->cdr);
//                 } else if(!strcmp(sym, "lambda")) {
//                     return lambda_ops(cc->cdr);
//                 } else if(!strcmp(sym, "quote")) {
//                     if(get_tag(cc->cdr) != CON)
//                         return box(ERR, WRONG_ARGUMENTS);
//                     return CELL(cc->cdr)->car;
//                 } else if(!strcmp(sym, "if")) {
//                     if(get_tag(cc->cdr) != CON || get_tag((cc = CELL(cc->cdr))->cdr) != CON)
//                         return box(ERR, WRONG_ARGS_NUMBER);
//                     if(get_tag(Eval(cc->car)) != NIL) {
//                         ast = CELL(cc->cdr)->car;
//                         goto restart;
//                         //return Eval((cc = CELL(cc->cdr))->car);
//                     }
//                     if(get_tag(cc->cdr) == CON) {
//                         cc = CELL(cc->cdr);
//                         if(get_tag(cc->cdr) == CON) {
//                             ast = CELL(cc->cdr)->car;
//                             goto restart;
//                             //return Eval(CELL(cc->cdr)->car);
//                         }
//                         return box(NIL, 0);
//                     } return box(ERR, WRONG_ARGUMENTS);
//                 } else if (!strcmp(sym, "do")) {
//                     first = nil;
//                     ast = cc->cdr;
//                     while(get_tag(ast) == CON) {
//                         cc = CELL(ast);
//                         first = Eval(cc->car);
//                         ast = cc->cdr;
//                     };
//                     return first;
//                 }
//             }
//             ast = eval_ast(ast);
//             cast = CELL(ast);
//             switch(get_tag(cast->car)) {
//                 case PRI:
//                     f = (Closure)get_val(cast->car);
//                     return f(cast->cdr);
//                 case CLO:
//                     if(!frame_lvl) {
//                         frame_lvl++;
//                         frame_new();
//                     } else {
//                         frame_rst();
//                     }
//                     defs = CELL(cast->car)->car;
//                     args = cast->cdr;
//                     while(get_tag(defs) == CON) {
//                         if (get_tag(args) == CON) {
//                             define_sym(CELL(CELL(defs)->car), CELL(args)->car);
//                             args = CELL(args)->cdr;
//                         } else { define_sym(CELL(CELL(defs)->car), nil); }
//                         defs = CELL(defs)->cdr;
//                     }
//                     ast = CELL(CELL(cast->car)->cdr)->car;
//                     goto restart;
//             }
//             return box(ERR, NOT_A_FUNCTION);
//             // PREVIOUSLY WAS: return apply(CELL(ast));
//     }
//     return eval_ast(ast);
// }
