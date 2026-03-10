/**
 * @file
 * @brief Collects definition of all primitives and special forms
 */

#include "functions.h"
#include "memory/heap.h"
#include "memory/stack.h"
#include "utility/box.h"
#include "utility/signals.h"
#include <stdio.h>
#include <string.h>

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
#define SPECIAL_FORMS_LIST  \
X(quote, Quote)

// Define all primitives
#define PRIMITIVES_LIST \
X(car,   Car)           \
X(cdr,   Cdr)           \
//X(cons,  Cons)          \
//X(?,     Type)

// Code generation macros, better not looking into this {{{
    
    // Internal: associative structure string name-procedure
    typedef struct {
        char* name;
        Function function;
    } FunctionMap;
    
    // Internal: forward declaration of all supported special forms and primitives
    // NOTE: special forms and primitives have the same signature
    #define X(name, function)   void specialForm##function(BoxRef,  Box);
    SPECIAL_FORMS_LIST
    #undef X
    #define X(name, function)   void primitive##function(BoxRef,    Box);
    PRIMITIVES_LIST
    #undef X
    
    // Internal: enumerate special forms and primitives to know the size of initial
    //           arrays
    #define X(name, function) SPECIAL_FORM_##function,
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
    // Special forms definition are searched straight from the array,
    // Primitives array is instead used to initialize the environment
    #define X(_name, _function) {.name = #_name, .function = specialForm##_function},
    FunctionMap specialFormsMap[SPECIAL_FORMS_SIZE] = {
        SPECIAL_FORMS_LIST
    };
    #undef X
    #define X(_name, _function) {.name = #_name, .function = primitive##_function},
    FunctionMap primitivesMap[PRIMITIVES_SIZE] = {
        PRIMITIVES_LIST
    };
    #undef X
    
// }}}

Function matchSpecialForm(char* name) {
    for (unsigned int i = 0; i < SPECIAL_FORMS_SIZE; i++) {
        if (strcmp(name, specialFormsMap[i].name) == 0)
            return specialFormsMap[i].function;
    }
    return NULL;
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
void specialFormQuote(BoxRef retBoxRef, Box argsBox) {
    if (getTag(&argsBox) != TAG_CONS) {
        *retBoxRef = boxSignal(SIGNAL_WRONG_ARGS_NUMBER);
        return;
    }
    *retBoxRef = ((Cons*)getValue(&argsBox))->car;
    return;

}

void initializeEnv() {
    for (unsigned int i = 0; i < PRIMITIVES_SIZE; i++) {
        // Make symbol into raw heap memory
        unsigned int len = strlen(primitivesMap[i].name);
        Box* rawRef = newRaw(len);
        setRaw(rawRef, primitivesMap[i].name);
        Box name = setBox((Value) rawRef, TAG_SYMBOL);
        Box definition = setBox((Value) primitivesMap[i].function, TAG_PRIMITIVE);
        defineSymbol(name, definition);
    }
}

////////////////////////////////////////////////////////////////////////////////
/// Primitives
////////////////////////////////////////////////////////////////////////////////

void primitiveCar(BoxRef retBoxRef, Box argsBox) {
    // Argument must be a cons, whose cdr is [anything] and car is another cons
    //                                        ^^^^^^^^
    //                                        Should be a Cons, but do I care?
    if (getTag(&argsBox) == TAG_CONS) {
        Cons* consRef = (Cons*) getValue(&argsBox);
        // Cannot get a car from a non-cons
        Tag tag = getTag(&consRef->car);
        if (tag == TAG_NIL) {
            *retBoxRef = boxNil();
            return;
        } else if (tag == TAG_CONS) {
            *retBoxRef = ((Cons*)getValue(&consRef->car))->car;
            return;
        }
    }
    *retBoxRef = boxSignal(SIGNAL_WRONG_ARGUMENTS);
    return;
}

void primitiveCdr(BoxRef retBoxRef, Box argsBox) {
    // Argument must be a cons, whose cdr is [anything] and car is another cons
    //                                        ^^^^^^^^
    //                                        Should be a Cons, but do I care?
    if (getTag(&argsBox) == TAG_CONS) {
        Cons* consRef = (Cons*) getValue(&argsBox);
        // Cannot get a car from a non-cons
        Tag tag = getTag(&consRef->car);
        if (tag == TAG_NIL) {
            *retBoxRef = boxNil();
            return;
        } else if (tag == TAG_CONS) {
            *retBoxRef = ((Cons*)getValue(&consRef->car))->cdr;
            return;
        }
    }
    *retBoxRef = boxSignal(SIGNAL_WRONG_ARGUMENTS);
    return;
}
