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

// FIXME: highly dependent on Eval, should embed it
/**
 * @brief Apply the evaluated list
 *
 * @param box ast to evaluate
 * @param formType true if form is leaf, false otherwise
 * @param hasFrame read/write, set when frame is created, if set do not create
 * @return result of applying value
 */
static inline Box GCapplyList(Box box, FormType *formType, int *hasFrame) {

    // Default is leaf (most comon)
    *formType = FORM_LEAF;

    // TODO: maybe duplicate
    // Early exit
    trace(&box);
    sig_check(box);

    // For convenience, keep function and argument box separated
    Box functionBox = getCar(&box),
        argumentBox = getCdr(&box);

    switch(getTag(&functionBox)) {
    case TAG_CLOSURE:
        // Make a new frame
        if(*hasFrame == 0) {
            *hasFrame = 1;
            framePush(SYM_STACK);
        }

        // Create binding in the new frame
        for(Box bindingBox = getCar(&functionBox)
                ; getTag(&bindingBox) == TAG_CONS
                ; argumentBox = getCdr(&argumentBox), bindingBox = getCdr(&bindingBox)) {

            // Just used to sig_check and trace, can copy straight into defineSymbol otherwise
            Box bindValueBox  = getCar(&argumentBox);

            trace(&bindValueBox);
            sig_check(bindValueBox);

            defineSymbol(SYM_STACK, getCar(&bindingBox), bindValueBox);
        }
        // NOTE: stops at shorter list
        // ((lambda (x y) (+ x y)) 1) ; x <- 1 , y <- ???
        // ((lambda (x) (+ x 1)) 1 2) ; x <- 1
        // TODO: what happens with SIGNALS?
        // ((lambda (x) (+ x 1)) 1 SIGNAL) ; ???

        // functionBox: ((x y) (+ x 1) (+ y 2))
        Box statementBox = getCdr(&functionBox),
            // statementBox: ((+ x 1) (+ y 2))
            resultBox = nil;

        // Do not deref
        if(getTag(&statementBox) == TAG_NIL){
            return nil;
        }

        // early check, prevents instant push-pop when body has a single statement
        Box nextBox = getCdr(&statementBox);
        if (getTag(&nextBox) == TAG_CONS) {
            // Iterate all but the last element
            for(pointerRegistryPush(&statementBox)
                    ; getTag(&nextBox) == TAG_CONS
                    ; statementBox = getCdr(&statementBox), nextBox = getCdr(&statementBox)) {

                resultBox = GCEval(getCar(&statementBox));

                // Stop at errors
                trace(&resultBox);
                sig_check(resultBox,
                    pointerRegistryPop();
                    return resultBox;
                );

            }
            pointerRegistryPop();
        }

        // Resulting form is composite -> yet to be evaluated
        *formType = FORM_COMPOSITE;
        // Return car of statements (which is the last one)
        return getCar(&statementBox);
        // Pop pointer registry and destroy frame
    case TAG_PRIMITIVE:
        // If function is a primitive, apply it to arguments and return
        functionBox = getPrimitive(functionBox)(argumentBox);
        // Primitives are leaf
        trace(&functionBox);
        return functionBox;
    default:
        // If function is neither primitive nor cons, it's an error
        logError("Cannot apply %s", strTag(getTag(&functionBox)));
        functionBox = boxSignal(SIGNAL_NOT_A_FUNCTION);
        // Signals are leaf
        trace(&functionBox);
        return functionBox;
    }
}

// Evaluate an ast
// TODO: change name
// This evaluates ast
//
// Atomics evaluate to themself
// Symbols are resolved
// Lists are evaluated element by element: (+ a b) -> (<prim@xx> 1 2)
/**
 * @brief Evaluate list element-by-element
 *
 * @param box Box referencing argument
 * @return
 */
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
        box = getSymbol(SYM_STACK, &box);
        trace(&box);
        return box;
    default:
        return box;
    }
}

// Evaluates an expression
Box GCEval(Box box) {
    // This eval is allowed to create a stack frame, setting "hasFrame" in the process
    int hasFrame = 0;
    for (;;) {

        Tag tag = getTag(&box);

        if (tag == TAG_CONS) {
            // Separate function and arguments for convenience
            Box functionBox = getCar(&box),
                argumentBox = getCdr(&box);

            // Special form to call (may call GC)
            Function GCspecialForm;
            // != 0 if special form is a leaf statement (define)
            // == 0 if it's not a leaf statement (if, do)
            // leaf statements can be returned
            // non-leaf statements return AST yet to be evaluated (in the current env)
            FormType leafStatement;

            // first element is a symbol & a special form
            if (getTag(&functionBox) == TAG_SYMBOL
                    && (GCspecialForm = matchSpecialForm(functionBox, &leafStatement))) {
                // Apply special form
                box = GCspecialForm(argumentBox);
                // If leaf statment, return, else continue
                if (leafStatement) goto evalReturn;
                continue;
            }

            // It's a cons but not a special form: must evaluate all elements
            // of the list (returns a new list of evaluated elements)
            // Uses previous env
            // (define a 1)(define b 2)(define add +)
            //
            // ; OK: (add a b) -> (<pri@01> 1 2)
            // ; OK: (a b 3) -> (1 2 3)
            // ; OK: (c 2 3) -> SIGNAL: symbol not defined
            //
            // Inside TAG_CONS branch, special forms evaluated restart loop,
            // if code reaches here it IS a cons which must be evaluated
            //
            // ; IMPOSSIBLE: 1 -> 1
            // ; IMPOSSIBLE: b -> 2
            //
            box = GCevalAst(box);
            // TODO: temporary, check signal
            trace(&box);
            sig_check(box,
                if(hasFrame) framePop(SYM_STACK););

            // Treat list as function, and apply it
            box = GCapplyList(box, &leafStatement, &hasFrame);
            if (leafStatement) goto evalReturn;
            continue;
        }
        // TODO: Is anything (cons or not) but surely NOT a special form
        box = GCevalAst(box);
    evalReturn:
        if (hasFrame) {
            framePop(SYM_STACK);
        }
        return box;
    }
}
