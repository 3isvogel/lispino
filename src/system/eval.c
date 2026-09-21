#include <utility/box.h>
#include <utility/signals.h>
#include <utility/log.h>
#include <memory/stack.h>
#include <memory/heap.h>

#include "functions.h"
#include "eval.h"

#include <stdio.h>
#include <string.h>

Box GCevalAst(Box box);
Box evalForm(Box box);

// Evaluates all statements but the last one
Box applyList(Box box) {

    trace(&box);
    sig_check(box);

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

            trace(&bindValueBox);
            sig_check(bindValueBox);

            defineSymbol(bindSymbolBox, bindValueBox);
        }
        // Evaluate all statements of a closure, return the last one

        Box statementBox = getCdr(&functionBox),
            resultBox = boxNil();

        for(pointerRegistryPush(&statementBox)
                ; getTag(&statementBox) == TAG_CONS
                ; statementBox = getCdr(&statementBox)) {

            // TODO: check local
            resultBox = GCEval(getCar(&statementBox));

            // If a signal arises, remember to pop both the env and the pointer registry
            trace(&resultBox);
            sig_check(resultBox,
                framePop();
                pointerRegistryPop();
            );
        }

        // TODO: optimize for tail-call
        framePop();
        pointerRegistryPop();

        trace(&resultBox);
        return resultBox;

    case TAG_PRIMITIVE:
        // getPrimitive(functionBox) returns a Function:
        // (function : Box -> Box), call it on argumetns
        // Cannot trace without breaking TCO
        return getPrimitive(functionBox)(argumentBox);
    default:
        logError("Cannot apply %s", strTag(getTag(&functionBox)));
        functionBox = boxSignal(SIGNAL_NOT_A_FUNCTION);
        trace(&functionBox);
        return functionBox;
    }
}

// Evaluate an ast
// TODO: change name
// This evaluates elemens of an ast independently:
//
// Atomics evaluate to themself
// Symbols are resolved
// Lists are evaluated element by element: (+ a b) -> (<prim@xx> 1 2)
Box GCevalAst(Box box) {

    Box headBox = boxNil(),
        tailBox = boxNil();

    switch (getTag(&box)) {
    case TAG_CONS:

        pointerRegistryPush(&headBox);
        pointerRegistryPush(&tailBox);
        pointerRegistryPush(&box);

        headBox = setBox((Value) GCnewCons(), TAG_CONS);
        tailBox = headBox;

        Box elementBox = getCar(&box);
        Box tempBox = GCEval(elementBox);
        // Check for signals
        trace(&tempBox);
        sig_check(tempBox,
            headBox = tempBox;
            goto evalAstReturn;
        );
        setCar(&tailBox, tempBox);
        box = getCdr(&box);

        while (getTag(&box) == TAG_CONS) {
            setCdr(&tailBox, setBox((Value) GCnewCons(), TAG_CONS));
            tailBox = getCdr(&tailBox);
            elementBox = getCar(&box);
            tempBox = GCEval(elementBox);
            // Check for signals
            trace(&tempBox);
            sig_check(tempBox,
                headBox = tempBox;
                goto evalAstReturn;
            );
            setCar(&tailBox, tempBox);
            box = getCdr(&box);
        }

    evalAstReturn:
        pointerRegistryPop();
        pointerRegistryPop();
        pointerRegistryPop();

        return headBox;

    case TAG_SYMBOL:
        box = getSymbol(&box);
        trace(&box);
        return box;
    default:
        return box;
    }
}

// Evaluates an expression
Box GCEval(Box box) {
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
                // May call GC
                box = specialForm(argumentBox);
                // If special form was a leaf type return
                if (formType == FORM_LEAF) {
                    return box;
                }
                continue;
            }

            // Not a special form: evaluate all arguments of the list
            box = GCevalAst(box);

            // applyList needs to be embedded here and cannot be made into a function call
            // as the recursion Eval -> applyList -> Eval -> applyList cannot be optimized
            box = applyList(box);

            return box;
        }
        // TODO: Is anything (cons or not) but surely NOT a special form
        box = GCevalAst(box);
        return box;
    }
}
