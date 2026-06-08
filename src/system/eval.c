#include <utility/box.h>
#include <utility/signals.h>
#include <utility/log.h>
#include <memory/stack.h>
#include <memory/heap.h>

#include "functions.h"
#include "eval.h"
#include "printer.h"

#include <stdio.h>
#include <string.h>

Box evalAst(Box box);
Box evalForm(Box box);

#define __printBox(file, line ,boxRef)  \
    do {                                \
        printf("      %s:%d:\n", file, line);   \
        Print(boxRef);                  \
    } while (0)

#define printBox(boxRef)    __printBox(__FILE__, __LINE__, boxRef)

// Evaluates all statements but the last one
Box applyList(Box box) {

    if (getTag(&box) == TAG_SIGNAL) return box;

    // NOTE: This does a useless check (I know that box is a Cons because it was
    //       returned from evalAst but who cares)
    Box functionBox = getCar(&box),
        argumentBox = getCdr(&box);

    switch(getTag(&functionBox)) {
    case TAG_CLOSURE:
        // TODO: reuse in case of tail-call
        // Create a new frame (env)
        framePush();

        // Create binding in the new frame (env)
        for(Box bindingBox = getCar(&functionBox)
                ; getTag(&bindingBox) == TAG_CONS
                ; argumentBox = getCdr(&argumentBox), bindingBox = getCdr(&bindingBox)) {

            Box bindSymbolBox = getCar(&bindingBox),
                bindValueBox  = getCar(&argumentBox);

            if (getTag(&bindValueBox) == TAG_SIGNAL) return bindValueBox;

            defineSymbol(bindSymbolBox, bindValueBox);
            printf("\n");
        }
        // Evaluate all statements of a closure, return the last one

        Box statementBox = getCdr(&functionBox),
            resultBox = boxNil();

        for(pointerRegistryPush(&statementBox)
                ; getTag(&statementBox) == TAG_CONS
                ; statementBox = getCdr(&statementBox)) {

            // TODO: check local
            resultBox = Eval(getCar(&statementBox));

            // If a signal arises, remember to pop both the env and the pointer registry
            if (getTag(&resultBox) == TAG_SIGNAL) {
                framePop();
                pointerRegistryPop();
                return resultBox;
            }
        }

        // TODO: optimize for tail-call
        framePop();
        pointerRegistryPop();

        return resultBox;

    case TAG_PRIMITIVE:
        // getPrimitive(functionBox) returns a Function:
        // (function : Box -> Box), call it on argumetns
        return getPrimitive(functionBox)(argumentBox);
    default:
        fprintf(stderr, "; APPLY LIST: ");
        return boxSignal(SIGNAL_NOT_A_FUNCTION);
    }
}

// Evaluate an ast
// TODO: change name
// This evaluates elemens of an ast independently:
//
// Atomics evaluate to themself
// Symbols are resolved
// Lists are evaluated element by element: (+ a b) -> (<prim@xx> 1 2)
Box evalAst(Box box) {
    
    Box headBox = boxNil(),
        tailBox = boxNil();

    switch (getTag(&box)) {
    case TAG_CONS:

        pointerRegistryPush(&headBox);
        pointerRegistryPush(&tailBox);
        pointerRegistryPush(&box);

        headBox = setBox((Value) newCons(), TAG_CONS);
        tailBox = headBox;

        Box elementBox = getCar(&box);
        Box tempBox = Eval(elementBox);
        // Check for signals
        if (getTag(&tempBox) == TAG_SIGNAL) {
            headBox = tempBox;
            goto evalAstReturn;
        }
        setCar(&tailBox, tempBox);
        box = getCdr(&box);

        while (getTag(&box) == TAG_CONS) {
            setCdr(&tailBox, setBox((Value) newCons(), TAG_CONS));
            tailBox = getCdr(&tailBox);
            elementBox = getCar(&box);
            tempBox = Eval(elementBox);
            // Check for signals
            if (getTag(&tempBox) == TAG_SIGNAL) {
                headBox = tempBox;
                goto evalAstReturn;
            }
            setCar(&tailBox, tempBox);
            box = getCdr(&box);
        }

    evalAstReturn:
        pointerRegistryPop();
        pointerRegistryPop();
        pointerRegistryPop();

        return headBox;

    case TAG_SYMBOL: return getSymbol(&box);
    default: return box;
    }
}

// Evaluates an expression
Box Eval(Box box) {
    for (;;) {

        Tag tag = getTag(&box);

        if (tag == TAG_CONS) {
            Box functionBox = getCar(&box),
                argumentBox = getCdr(&box);
    
            Function specialForm;
            FormType formType;
            // Handle special forms
            if (getTag(&functionBox) == TAG_SYMBOL
                    && (specialForm = matchSpecialForm(functionBox, &formType))) {
                // Apply special form
                box = specialForm(argumentBox);
                // If special form was a leaf type return
                if (formType == FORM_LEAF) {
                    return box;
                }
                continue;
            }

            // Not a special form: evaluate all arguments of the list
            box = evalAst(box);

            return applyList(box);
        }
        // TODO: Is anything (cons or not) but surely NOT a special form
        return evalAst(box);
    }
}
