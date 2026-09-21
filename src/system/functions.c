/**
 * @file
 * @brief Collects definition of all primitives and special forms
 */

#include "functions.h"
#include "memory/heap.h"
#include "memory/private.h"
#include "memory/stack.h"
#include "system/eval.h"
#include "system/printer.h"
#include "utility/box.h"
#include "utility/log.h"
#include "utility/signals.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define __printBox(file, line ,box)  \
    do {                                \
        printf("%s:%d:", file, line);   \
        Print(box);                  \
    } while (0)

#define printBox(box)    __printBox(__FILE__, __LINE__, box)

/*
 * Each special form must be registered in the SPECIAL_FORMS_LIST as
 * X(<unique special form name>, <unique identifier>), the procedure must be
 * specified in a function with signature:
 *     void specialForm<identifier>(BoxRef returnRef, Box arguments);
 *
 * Same way, all primitives must registered in the PRIMITIVES_LIST as
 * X(<primitive name>, <unique identifier>), the procedure must be
 * specified in a function with signature:
 *     void primitive<identifier>(BoxRef returnRef, Box arguments);
 */

// Define all special forms
#define SPECIAL_FORMS_LIST      \
X(quote,     , Quote,  LEAF)       \
X(if,      GC, If,     COMPOSITE)  \
X(do,      GC, Do,     COMPOSITE)  \
X(lambda,    , Lambda, LEAF)       \
X(define,    , Define, LEAF)       \

// Define all primitives
#define PRIMITIVES_LIST     \
X(car,      Car)            \
X(cdr,      Cdr)            \
/*X(cons,  Cons)*/          \
X(?,        Type)           \
X(sym,      Sym)            \
X(form,     Form)           \
X(+,        IntAdd)         \
X(eq,       Eq)             \
X(println,  Println)


// Code generation macros, better not looking into this {{{

    // Internal: associative structure string name-procedure
    typedef struct {
        char* name;
        Function function;
    } FunctionMap;

    // Internal: forward declaration of all supported special forms and primitives
    // NOTE: special forms and primitives have the same signature
    #define X(name, prefix, function, type)   Box prefix##specialForm##function(Box box);
    SPECIAL_FORMS_LIST
    #undef X
    #define X(name, function)   Box primitive##function(Box box);
    PRIMITIVES_LIST
    #undef X

    // Internal: enumerate special forms and primitives to know the size of initial
    //           arrays
    #define X(name, prefix, function, type) SPECIAL_FORM_##function,
    typedef enum {
        SPECIAL_FORMS_LIST
        SPECIAL_FORMS_SIZE
    } SpecialForm;
    #undef X
    #define X(name, function) PRIMITIVE_##function,
    typedef enum {
        PRIMITIVES_LIST
        PRIMITIVES_SIZE
    } Primitive;
    #undef X

    // Internal: Arrays mapping names to special form definitions and primitives
    // Special forms definitions are searched straight from the array,
    // Primitives array is instead used to initialize the environment
    #define X(_name, prefix, _function, type) {.name = #_name, .function = prefix##specialForm##_function},
    FunctionMap specialFormsMap[SPECIAL_FORMS_SIZE] = {
        SPECIAL_FORMS_LIST
    };
    #undef X
    #define X(_name, _function) {.name = #_name, .function = primitive##_function},
    FunctionMap primitivesMap[PRIMITIVES_SIZE] = {
        PRIMITIVES_LIST
    };
    #undef X

    // Internal: For special forms only: Array mapping function to one of the
    // two values FORM_LEAF (1) & FORM_COMPOSITE (0), indicating if the function
    // returns an evaluated expression (quote, lambda, define, etc) or an
    // expression yet to be evaluated (if, do, etc.) this is used to instruct
    // Eval function on how to treat the returned value, returning it or
    // evaluating it further

    // Creates a bit mask where setting bit in position i (1 << i) means the
    // i-th function is a leaf one
    // NOTE: God forbid me
    #define X(_name, prefix, function, type) | (FORM_##type << SPECIAL_FORM_##function)
    unsigned long long int specialFormsType = 0 SPECIAL_FORMS_LIST;
    #undef X 

    static_assert(SPECIAL_FORMS_SIZE <= (sizeof(specialFormsType)*8),
            "Too many special forms, the special form types map is overflowing, you must change implementation");
// }}}

Box primitiveUnknown(Box box);

Function getPrimitive(Box box) {
    if (getTag(&box) != TAG_PRIMITIVE) return primitiveUnknown;
    Value functionId = getValue(&box);
    if (functionId >= PRIMITIVES_SIZE) return primitiveUnknown;
    return primitivesMap[functionId].function;
}

Function matchSpecialForm(Box box, FormType* isLeafStatement) {
    char* name = getRaw(box);
    for (unsigned int i = 0; i < SPECIAL_FORMS_SIZE; i++) {
        if (strcmp(name, specialFormsMap[i].name) == 0) {
            // NOTE: faster but not explicatory
            // *isLeafStatement = specialFormsType & (1 << i);
            // NOTE: slower but more explicatory
            *isLeafStatement = (specialFormsType & (1 << i)) ? FORM_LEAF : FORM_COMPOSITE;
//            logInfo("Special form \"%s\" is: %s", name, isLeafStatement ? "LEAF" : "Composite");
            return specialFormsMap[i].function;
        }
    }
    return NULL;
}

void GCinitializeEnv() {
    for (unsigned int i = 0; i < PRIMITIVES_SIZE; i++) {
        // Make symbol into raw heap memory
        unsigned int len = strlen(primitivesMap[i].name);
        Box* rawRef = GCnewRaw(len);
        setRaw(rawRef, primitivesMap[i].name);
        Box name = setBox((Value) rawRef, TAG_SYMBOL);
        Box definition = setBox((Value) i, TAG_PRIMITIVE);
        defineSymbol(name, definition);
    }
}

// TODO: consider if using a wrapper function "applySpecialForm() that pushes
// pointers on stack if needed

////////////////////////////////////////////////////////////////////////////////
/// 
/// Below are definitions of all special forms and primitives
///
////////////////////////////////////////////////////////////////////////////////

// NOTE: Function type requires two parameters: argsBox and box, these are
// NOTE: respectively: a reference to the destination box and a copy of the
// NOTE: argument box: acting as the return value and arguments,
// NOTE: if a function or special form allocates memory, then it's
// NOTE: THEIR responsibility to [NOTE] PUSH POINTER TO THE REGISTRY [NOTE]

////////////////////////////////////////////////////////////////////////////////
/// Special forms
////////////////////////////////////////////////////////////////////////////////

// "quote" special form
Box specialFormQuote(Box box) {
    return getCar(&box);
}

// "if" special form
Box GCspecialFormIf(Box box) {

    // Condition for the if statement
    Box conditionBox = getCar(&box),
        statementsBox = getCdr(&box);

    pointerRegistryPush(&conditionBox);
    pointerRegistryPush(&statementsBox);

        // Evaluate condition
        conditionBox = GCEval(conditionBox);

    pointerRegistryPop();
    pointerRegistryPop();

    // If signal occured in condition, return it:
    sig_check(conditionBox);

    // NOTE: No need to eval, can return and eval outside

    // True branch
    if (getTag(&conditionBox) != TAG_NIL) return GCEval(getCar(&statementsBox));

    // False branch
    statementsBox = getCdr(&statementsBox);
    if (getTag(&statementsBox) == TAG_NIL) return nil;
    statementsBox = getCar(&statementsBox);
    return GCEval(statementsBox);
}

// "do" special form
Box GCspecialFormDo(Box box) {

    Box currentBox = getCar(&box),
        remainingBox = getCdr(&box);
    pointerRegistryPush(&currentBox);
    pointerRegistryPush(&remainingBox);

    // Evaluate until a box is remaining
    while (getTag(&remainingBox) == TAG_CONS) {

        // Get car of the first element in args list
        currentBox = GCEval(currentBox);

        // If something happened: return
        sig_check(currentBox,
            pointerRegistryPop();
            pointerRegistryPop();
        );

        currentBox = getCar(&remainingBox);
        remainingBox = getCdr(&remainingBox);
    }

    // Ok, you can terminate a do with anything, not just a nil (just not a cons)

    pointerRegistryPop();
    pointerRegistryPop();

    // NOTE: no need to eval, can return and eval outside

    return GCEval(currentBox);
}

Box specialFormLambda(Box box) {
    // A closure (lambda) must be a cons, which car is a lst of symbols (env),
    // and the cdr must be a cons (function body), lambdas will act as if all
    // statements are inside a (do) statement

    // Checks that binding is either nil or a list of symbols
    Box bindingsBox = getCar(&box);

    if (getTag(&bindingsBox) != TAG_NIL && getTag(&bindingsBox) != TAG_CONS)
        return boxSignal(SIGNAL_LAMBDA_ARGS);

    for(; getTag(&bindingsBox) == TAG_CONS; bindingsBox = getCdr(&bindingsBox)) {
        Box bindingBox = getCar(&bindingsBox);
        if (getTag(&bindingBox) != TAG_SYMBOL) return boxSignal(SIGNAL_LAMBDA_ARGS);
    }

    // Statements can potentially be anything, if it's a cons it's a proper
    // lambda which will perform actions, if it's a nil it will simply return
    // nothing, if it's something else I will just treat it as a nil

    // Lambda is just the original cons tagged to be recognized as closure (might
    // be not needed)
    return setBox(getValue(&box), TAG_CLOSURE);
}

// Associate a value obtained by evaluating the second argument to a symbol
Box specialFormDefine(Box box) {
    Box symbolBox = getCar(&box);

    if (getTag(&symbolBox) != TAG_SYMBOL) {
        return boxSignal(SIGNAL_WRONG_ARGUMENTS);
    }

    box = getCdr(&box);
    box = getCar(&box);
    pointerRegistryPush(&symbolBox);
    box = GCEval(box);
    pointerRegistryPop();

    sig_check(box);

    return defineSymbol(symbolBox, box);
}

////////////////////////////////////////////////////////////////////////////////
/// Primitives
////////////////////////////////////////////////////////////////////////////////

Box primitiveUnknown(Box box) {
    logError("Primitive not found");
    box = boxSignal(SIGNAL_BAD_REFERENCE);
    trace(&box);
    return box;
}

Box primitiveCar(Box box) {
    // Argument must be a cons, whose cdr is [anything] and car is another cons
    //                                        ^^^^^^^^
    //                                        Should be a Cons, but do I care?
    Box argumentBox = getCar(&box);
    return getCar(&argumentBox);
}

Box primitiveCdr(Box box) {
    // Argument must be a cons, whose cdr is [anything] and car is another cons
    //                                        ^^^^^^^^
    //                                        Should be a Cons, but do I care?
    Box argumentBox = getCar(&box);
    return getCdr(&argumentBox);
}

Box primitiveType(Box box) {
    Box argBox = getCar(&box);
    return setBox(getTag(&argBox), TAG_INT);
}

Box primitiveSym(Box box) {
    printf("PrimSym");
    for (int i = 0; i < stack.head; i++) {
        printf("%s ", getRaw(stack.data[i].car));
    }
    printf("\n");
    return nil;
}

Box primitiveForm(Box box) {
    printf("PrimForm");
    for (int i = 1; i < SPECIAL_FORMS_SIZE; i++) {
        printf("%s ", specialFormsMap[i].name);
    }
    printf("\n");
    return nil;
}

Box primitiveIntAdd(Box box) {
    int acc = 0;
    Box iter = getCar(&box),
        val = iter;
    if (getTag(&val) == TAG_INT) {
        acc = getValue(&val);

        for (iter = getCdr(&box), val = getCar(&iter);
                getTag(&iter) != TAG_NIL;
                iter = getCdr(&iter), val = getCar(&iter)) {
            if (getTag(&val) != TAG_INT) {
                logError("Cannot add %s", strTag(getTag(&val)));
                box = boxSignal(SIGNAL_WRONG_TYPE);
            }
            acc += getValue(&val);
        }
    }
    return setBox(acc, TAG_INT);
}

int consEq(BoxRef a, BoxRef b) {
    if (a == b) return 1;
    todo("consEq not implemented");
    return 0;
}

Box primitiveEq(Box box) {
    Box first, iter, val;

    int eq = 1;

    for(first = getCar(&box), iter = getCdr(&box), val = getCar(&iter);
            getTag(&iter) != TAG_NIL;
            iter = getCdr(&iter), val = getCar(&iter)) {

        sig_check(iter);
        // TODO: check if it's equivalent
        if (getTag(&iter) == TAG_SIGNAL) return boxSignal(getValue(&iter));

        const Tag ta = getTag(&first), tb = getTag(&val);
        const Value va = getValue(&first), vb = getValue(&val);
        if (ta != tb) {
            eq = 0;
        } else if (ta == TAG_INT) {
            if (va != vb) eq = 0;
        } else if (ta == TAG_CONS) {
            eq = consEq((BoxRef) NULL, (BoxRef) NULL);
        // Or any other strcmp
        } else if (ta == TAG_STRING) {
            todo("string eq not implemented");
        }
        if(!eq) return nil;
    }
    return setBox(1, TAG_INT);
}

Box primitivePrintln(Box box) {
    Box val, iter;
    for(iter = box, val = getCar(&iter);
            getTag(&iter) != TAG_NIL;
            iter = getCdr(&iter), val = getCar(&iter)) {
        sig_check(box);
        innerPrint(val, 1);
    }
    printf("\n");
    return nil;
}
