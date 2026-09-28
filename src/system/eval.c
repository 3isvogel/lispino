#include <utility/box.h>
#include <utility/signals.h>
#include <utility/log.h>
#include <memory/stack.h>
#include <memory/heap.h>

#include "functions.h"
#include "system/printer.h"
#include "eval.h"

#include <stdio.h>
#include <string.h>

#define show(box, ...) doDebug( \
    logDebug(" v " __VA_ARGS__); \
    Print(box, stderr); \
)

/**
 * @brief Evaluate list element-by-element
 *
 * @param box Box referencing argument
 * @return Address of binding
 */
BoxRef GCevalBinding(Box box, unsigned int* bindingSize) {

    // evalAst is called exclusively with cons
    // TODO: consider implementing stack frames using Box(TAG_INT, base_pointer)
    BoxRef bindings = bindStackReserveN(0);     /// <- hacky, should actually use a framed stack
    BoxRef binding = bindings;
    pointerRegistryPush(&box);
    for(*bindingSize = 0, binding = bindings
        ; getTag(binding) == TAG_SIGNAL         // Might not be standard, but is an easier check
          || getTag(&box) == TAG_CONS           // This allows me to terminate lists with anything:
        ; box = getCdr(&box))                   // (+ a b . c) == (+ a b)
    {
        binding = bindStackReserve();           ///
        *bindingSize = (*bindingSize+1);        /// <- Should use framed stack
        *binding = GCEval(getCar(&box));
        doDebug(
            Print(*binding, stderr);
            logDebug(" ^ Value");
        );
        trace(binding);
    }
    if (getTag(binding) == TAG_SIGNAL)          // Early exit, return the value instead
        bindings = binding;
    pointerRegistryPop();
    return bindings;
}

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
    sig_check(box);

    // TODO: remove
    if (unlikely(getTag(&box) != TAG_CONS)) {
        logError("Expecting Cons, got %s", strTag(getTag(&box)));
    }
    assert(getTag(&box) == TAG_CONS);

    unsigned int bindingSize;
    BoxRef bindings = GCevalBinding(box, &bindingSize);
    sig_check(bindings[0], bindStackPopN(bindingSize); );
    doDebug(
        fprintf(stderr, "[%2d]: ", bindingSize);
        for (int i = 0; i < bindingSize; i++) {
            innerPrint(bindings[i], NON_READABLE, stderr);
            fprintf(stderr, " ");
        }
        fprintf(stderr, "\n");
        logDebug(" ^ Evaluated");
    );

    // Default is leaf (catches primitives and atomics)
    *formType = FORM_LEAF;

    switch(getTag(bindings)) {
    case TAG_CLOSURE:
        // Make a new frame
        if(*hasFrame == 0) {
            *hasFrame = 1;
            framePush();
        }

        int i = 0;
        Box symbols;
        if (bindingSize > 0) {  // TODO: can I avoid check?
            for(symbols = getCar(&bindings[0]), i = 1
                ; getTag(&symbols) == TAG_CONS // && i < bindingSize
                ; symbols = getCdr(&symbols), ++i) {

                const Box symbol = getCar(&symbols);
                assert(getTag(&symbol) == TAG_SYMBOL);
                assert(getTag(&bindings[i]) != TAG_SIGNAL);
                if (i < bindingSize) defineSymbol(symbol, bindings[i]);
                else defineSymbol(symbol, nil);

                doDebug(
                    Box _box = symbol;
                    Print(getSymbol(&_box), stderr);
                    logDebug(" ^ Bound to \"%s\"", getRaw(symbol));
                );
            }
        }
        // Reference first statement
        Box statementBox = getCdr(&bindings[0]);
        // statementBox = getCdr(&statementBox);
        doDebug(
            Box s; int i;
            for(s = statementBox, i=0; getTag(&s) == TAG_CONS; s = getCdr(&s), i++) {
                fprintf(stderr, "%2d: ", i);
                Print(getCar(&statementBox), stderr);
            }
            logDebug(" ^ Lambda statements");
        );
        // Can pop binding stack early
        bindStackPopN(bindingSize);

        // If it's nil will skip for loop
        if (getTag(&statementBox) == TAG_NIL) { break; }

        *formType = FORM_COMPOSITE;

        Box nextBox = getCdr(&statementBox);
        // Statement cons is either cons or nil
        // Always doe push-pop, even on nil
        for(pointerRegistryPush(&statementBox)
                ; getTag(&nextBox) == TAG_CONS
                ; statementBox = getCdr(&statementBox), nextBox = getCdr(&statementBox)) {

            box = GCEval(getCar(&statementBox));

            // Stop at errors
            trace(&box);
            sig_check(box,
                pointerRegistryPop();
                return box;
            );
            // Resulting form is composite -> yet to be evaluated
        }
        pointerRegistryPop();
        // Return car of statements (which is the last one)
        box = getCar(&statementBox);
        break;
    case TAG_PRIMITIVE:
        // If function is a primitive, apply it to arguments and return
        box = getPrimitive(bindings[0])((BoxArgs){
                .data = &bindings[1],.size = bindingSize-1});
        bindStackPopN(bindingSize);
        // Primitives are leaf
        break;
    default:
        // If function is neither primitive nor cons, it's an error
        // Raise a signal
        logError("Cannot apply %s", strTag(getTag(bindings)));
         bindStackPopN(bindingSize);
        box = boxSignal(SIGNAL_NOT_A_FUNCTION);
        break;
    }
    trace(bindings);
    return box;
}

// Evaluates an expression
Box GCEval(Box box) {
    // This eval is allowed to create a stack frame, setting "hasFrame" in the process
    int hasFrame = 0;
    Box functionBox, argumentBox;
    // != 0 if special form is a leaf statement (define)
    // == 0 if it's not a leaf statement (if, do)
    // leaf statements can be returned
    // non-leaf statements return AST yet to be evaluated (in the current env)
    FormType leafStatement;
    for (;;) {

        show(box, "Eval");

        Tag tag = getTag(&box);

        if (tag == TAG_SYMBOL) {
            logDebug(" ^ Symbol");
            box = getSymbol(&box);
        } else if (tag == TAG_CONS) {
            // Separate function and arguments for convenience
            functionBox = getCar(&box);
            argumentBox = getCdr(&box);

            // Special form to call (may call GC)
            SpecialForm GCspecialForm;
            if (getTag(&functionBox) == TAG_SYMBOL
                    && (GCspecialForm = matchSpecialForm(functionBox, &leafStatement))) {
                logDebug(" ^ Special form - (%s ...)", getRaw(functionBox));
                // First element is a symbol & a special form
                // Apply special form
                box = GCspecialForm(argumentBox);
            } else {
                logDebug(" ^ Function call - (%s ...)", getRaw(functionBox));
                // Not a special form, apply it, can be a nonsymbol?
                box = GCapplyList(box, &leafStatement, &hasFrame);
                doDebug(
                    Print(box, stderr);
                    logDebug(" ^ Result ");
                );
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
                // TODO: Instead of calling GCevalAst
                // (a b 3) -> (1 2 3)
                // solve the symbols and save them onto secondary (bindings) stack
                // If call is a primitive -> rewrite primitives to expect arguments from a stack
                // If call is a closure   -> - Copy symbols from temporary bind frame onto main stack
                //                           - Delete frame
                //                           - Normal call
                //                           - Cannot use a single stack, otherwise functions like
                //                           - ((lambda (x y)) (+ y 1) (+ x 1)) clash as they may re-utilize
                //                             the same symbols, yet, I know that the cons generated by
                //                             GCevalAST is not going to live long (is discarded after apply)
                //
                // Do not allocate a new list for call
                // Treat list as function, and apply it
            }
            // If leaf statment, return, else continue
            if (!leafStatement) continue;
        }
        if (hasFrame) { framePop(); }
        return box;
    }
}
