/**
 * @file
 * @brief Parser and lexer implementation
 */

#include "utility/log.h"
#include <utility/signals.h>
#include <utility/box.h>

#include <memory/mem.h>
#include <memory/heap.h>

#include <system/lexer.h>

#include <stdio.h>
#include <string.h>

/**
 * @brief Maps a TokenType to the respective Tag
 *
 * Returns TYPE_SIGNAL if conversion is not possible
 *
 * @param tokenType 
 * @return 
 */
// FIXME: This whole translation could be avoided if I enforce that
// FIXME: 
// FIXME: TTYPE_x = TAG_x
// FIXME: 
Tag ttypeToTag(TokenType tokenType) {
    // TODO: can I make translation inside TOKEN_TYPE_LIST? eg:
    //       X(STRING,TAG_STRING) X(LPAR,TAG_SIGNAL)
    switch(tokenType) {
    case TTYPE_SYMBOL:  return TAG_SYMBOL;
    case TTYPE_STRING:  return TAG_STRING;
    case TTYPE_LABEL:   return TAG_LABEL;
    case TTYPE_INT:     return TAG_INT;

    // This function is intended to translate similar types (string, label,
    // symbols) to corresponding tag type, but declaration implies it can work
    // with any type, the correct way is to catch different types earlier
    // (like nil and dot) but these cases are included anyway for safety
    // Since this checks are supposed to fail anyway, put them at the end,
    // if you enter the cases listed below you can expect to almost certainly
    // return a TAG_SIGNAL

    case TTYPE_NIL:     return TAG_NIL;
    default:
        logWarning("You might have forgot to implement a type translation");
                        return TAG_SIGNAL;
    }
}

// LISP simplified BNF
//
// S -> A | (L)                 <-- readForm
// L -> \epsilon | SL           <-- readList

void readForm(BoxRef boxRef);
void readList(BoxRef boxRef);

void specialTransform(BoxRef boxRef, char* name) {
    
    // Copy string inside heap
    // TODO: could skip the copy if string already inside the heap boundaries
    unsigned int len = strlen(name);
    BoxRef rawRef = newRaw(len);
    *boxRef = setRaw(rawRef, name);
    if (getTag(boxRef) != TAG_SIGNAL) {
        setTag(boxRef, TAG_SYMBOL);

        // Make a new cons, assign its car to the special symbol and save cons in
        // boxRef
        Cons *cons = newCons();
        cons->car = *boxRef;
        *boxRef = setBox((Value)cons, TAG_CONS);
    
        // Read value of next form
        next();
        Box value = boxNil();
        pointerRegistryPush(&value);
        readForm(&value);
        pointerRegistryPop();
        
        // Append it after special symbol
        cons = newCons();
        cons->car = value;
        // default cdr = nil
        ((Cons*)getValue(boxRef))->cdr = setBox((Value) cons, TAG_CONS);
    }
}

void readForm(BoxRef boxRef) {
    // Save in pointer registry for automatic update on GC
    // Every registered Box MUST be initialized to prevent unwanted behavior
    BoxRef rawRef;

    switch(token.type) {
    case TTYPE_LPAR:
        // call readList
        next();
        readList(boxRef);
        // Keeping signals separated to later differenciate them
        if (getTag(boxRef) == TAG_SIGNAL) {
            *boxRef = boxSignal(SIGNAL_SYNTAX_ERROR);
        } else if (token.type != TTYPE_RPAR) {
            *boxRef = boxSignal(SIGNAL_SYNTAX_ERROR);
        }
       break;
    // read atomic
    // TODO: Could handle these types in a better way (default: + if-else) but
    //       I think it is enough to just map the TTYPE values to the TAG values
    case TTYPE_STRING:
    case TTYPE_SYMBOL:
    case TTYPE_LABEL:
        // Request new memory and fill with value

        // Might call GC
        rawRef = newRaw(token.len);

        *boxRef = setRaw(rawRef, token.text);
        // If assignment didn't fail populate box with value and tag
        if (getTag(boxRef) != TAG_SIGNAL) {
            // Should check that the returned tag is not a signal, but since this
            // call relies under TTYPE_STRING/SYMBOL/LABEL there is no need to check
            setTag(boxRef, ttypeToTag(token.type));
        }
        // Otherwise keep signal
        break;
    case TTYPE_DOT:
        // If a dot appears here then a list is malformed
    case TTYPE_RPAR:
        // A form is either an atomic type or the beginning of a list
        *boxRef = boxSignal(SIGNAL_SYNTAX_ERROR);
        break; 

    // Implementing other special characters
    case TTYPE_QUOTE:
        // Single quoatation (skips eval ~) is resolved at read time: a form " ' <exp> " is constructed as "(quote <exp>)"
        // Keep separated implementation of special symbols transforms
        specialTransform(boxRef, "quote");
        break;
    case TTYPE_INT:
    default:
        setValue(boxRef, token.value);
        setTag(boxRef, ttypeToTag(token.type));
    }
}

void readList(BoxRef boxRef) {

    // This function could be implemented simply as a 
    // car = readForm();
    // next();
    // cdr = readList();
    // And then constructing just a single cons, leaving the burden of keeping
    // references to recursion, however, doing so would result in possibly very
    // deep recursion (list would be scanned recursively, to avoid this
    // implement lists iteratively, this is simple computer science 101 list
    // iterative creation, using only 3 pointers:
    //
    // TODO: Another option that might make the operation easier is
    // readList(BoxRef head, BoxRef value, BoxRef value);
    // And using tail call

    // If first token is a right parenthesis, empty list evaluates to nil,
    // skip list creation alltogether
    if (token.type == TTYPE_RPAR) {
        *boxRef = boxNil();
        return;
    }
    
        // Temporary value to be added in a car
    Box value = boxNil(),
        // Reference to the previous cons, temporary reference to list's tail
        prev  = boxNil();


    pointerRegistryPush(&value);
    pointerRegistryPush(&prev);

    // Read next form and assign it as car of a new cons
    readForm(&value);
    next();
    // If value obtained is a signal discard list and bubble up the value
    signalPass(boxRef, &value);
    Cons* consRef = newCons();
    consRef->car = value;

    // Reference it from a registered box so it will survive GC, this will
    // be the root of the tree returned
    *boxRef = setBox((Value) consRef, TAG_CONS);
    // Additional reference used for iteratively scan list
    prev = setBox((Value) consRef, TAG_CONS);

    // Keep going until the end of the current list or an explicit cdr is
    // signaled by the "."
    while(token.type != TTYPE_RPAR && token.type != TTYPE_DOT) {

        readForm(&value);
        next();
        // If value is a signal assign it to boxRef
        signalPass(boxRef, &value);

        // reserve a new cons (will continue the list
        consRef = newCons();
        // Set car value of the folowing Cons
        consRef->car = value;

        // Link previous Cons with the new one
        Cons* prevConsRef = (Cons*)getValue(&prev);
        prevConsRef->cdr = setBox((Value) consRef, TAG_CONS);

        // Move "prev" to point to new Cons (Tag stays the same)
        setValue(&prev, (Value) consRef);
    }

    // Close the list setting last cdr
    // If the token is a dot, then specify the cdr, otherwise it's just a
    // nil box
    if (token.type == TTYPE_RPAR) {
        // Close with a null, do not consume last ')'
        ((Cons*)getValue(&prev))->cdr = boxNil();
    } else if (token.type == TTYPE_DOT) {
        // If the next token is a dot consume it, append the value to
        // cdr instead of car and do not allocate new Cons
        next();
        readForm(&value);
        // If value is a signal assign it to boxRef
        signalPass(boxRef, &value);
        // Need to extract value from "iterative" as consRef might be
        // invalid
        ((Cons*)getValue(&prev))->cdr = value;
        // consume '.' and Form, leave final ')', is checked by readForm
        next();
    } else {
        // This should not happen, but I will keep it as a guard
        *boxRef = boxSignal(SIGNAL_SYNTAX_ERROR);
    }

    pointerRegistryPop();
    pointerRegistryPop();
}

Box Read() {

    Box box = boxNil();
    next();

    pointerRegistryPush(&box);

    readForm(&box);

    pointerRegistryPop();

    return box;
}
