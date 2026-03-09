/**
 * @file
 * @brief Parser and lexer implementation
 */

#include "utility/log.h"
#include <utility/signals.h>
#include <utility/box.h>

#include <memory/mem.h>
#include <memory/heap.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Used by the lexer to communicate types of tokens to the parser
#define TOKEN_TYPE_LIST \
X(LPAR)                 \
X(RPAR)                 \
X(STRING)               \
X(SYMBOL)               \
X(LABEL)                \
X(DOT)                  \
X(NIL)                  \
X(QUOTE)

#define X(x) TTYPE_##x,
typedef enum {
    TOKEN_TYPE_LIST \
    TOKEN_TYPE_SIZE
} TokenType;
#undef X

/**
 * @brief Maps a TokenType to the respective Tag
 *
 * Returns TYPE_SIGNAL if conversion is not possible
 *
 * @param tokenType 
 * @return 
 */
Tag ttypeToTag(TokenType tokenType) {
    // TODO: can I make translation inside TOKEN_TYPE_LIST? eg:
    //       X(STRING,TAG_STRING) X(LPAR,TAG_SIGNAL)
    switch(tokenType) {
    case TTYPE_SYMBOL:  return TAG_SYMBOL;
    case TTYPE_STRING:  return TAG_STRING;
    case TTYPE_LABEL:   return TAG_LABEL;

    // This function is intended to translate similar types (string, label,
    // symbols) to corresponding tag type, but declaration implies it can work
    // with any type, the correct way is to catch different types earlier
    // (like nil and dot) but these cases are included anyway for safety
    // Since this checks are supposed to fail anyway, put them at the end,
    // if you enter the cases listed below you can expect to almost certainly
    // return a TAG_SIGNAL

    case TTYPE_NIL:     return TAG_NIL;
    default:            return TAG_SIGNAL;
    }
}

typedef long long int TokenValue;

typedef struct {
    char*           text;
    unsigned int    len;
    TokenType       type;
    TokenValue      value;
    unsigned int    bufferSize;
} Token;
Token token;

void destroyParser() {
    free(token.text);
}

/**
 * @brief Allocate memory for the parser
 *
 * @param maximumTokenSize 
 * @return pointer to allocated buffer, used by memory management to free it
 * later
 */
char* createParser(unsigned int bufferSize) {
    if (token.text != NULL) destroyParser();

    char* buffer = (char*)halloc(bufferSize + 1);

    token = (Token) {
        .text = buffer,
        .len = 0,
        .bufferSize = bufferSize,
        .type = TTYPE_NIL,
        .value = 0,
    };
    return buffer;
}

/**
 * @brief Append character to the token safely
 *        TODO: (needs refining)
 *
 * @param character 
 */
void putch(char character) {
    if (token.len == token.bufferSize) {
        logError("Token: %.*s", token.len, token.text);
        fail(SIGNAL_TOKEN_TOO_LONG);
    }
    token.text[token.len ++ ] = character;
}

void clearToken() {
    token.len = 0;
}

/**
 * @brief Consume input until a matching token is found
 *
 * @return TODO: nothing at the moment
 */
int cc = '\0';
void next() {

    clearToken();

    do {
        
        if (cc == EOF) {
            // TODO: actually handle this
            fail(SIGNAL_EOF_REACHED);
    
        // TODO: handle special characters, used as abbreviations for longer
        // forms, returns
        // just a single character
        } else if (cc == '(') {
            token.type = TTYPE_LPAR;
            putch(cc);
            goto lexerConsumeAndReturn;
        } else if (cc == ')') {
            token.type = TTYPE_RPAR;
            putch(cc);
            goto lexerConsumeAndReturn;
        } else if (cc == '.') {
            token.type = TTYPE_DOT;
            putch(cc);
            goto lexerConsumeAndReturn;
        } else if (cc == '\'') {
            token.type = TTYPE_QUOTE;
            putch(cc);
            goto lexerConsumeAndReturn;
        } else if (cc ==  '"') {
            cc = getchar();
            while (cc != '"'){
                if (cc != '\\') {
                    putchar(cc);
                } else {
                    cc = getchar();
                    static const char *escapeChars = "abtnvfr";
                    const char *escapedLetter = strchr(escapeChars, cc);
                    putchar(escapedLetter ? escapedLetter - escapeChars : cc);
                }
                cc = getchar();
            }
            token.type = TTYPE_STRING;
            goto lexerConsumeAndReturn;
            // Keep adding character until a non-escaped " is found
        } else if (cc ==  ';') {
            while((cc = getchar()) != '\n') {};
            // Also consume '\n'
        // TODO: recognize symbols
        } else if (cc >= '*' && cc <= '~') {
            do {
                putch(cc);
                cc = getchar();
            } while(cc >= '*' && cc <= '~');
            if (token.text[0] == ':')
                token.type = TTYPE_LABEL;
            else
                token.type = TTYPE_SYMBOL;
            goto lexerReturn;
        } else if (cc >= '!' && cc <= '\'') {
            logError("%c (%x)", cc, cc);
            todo("Implement special characters");
        }
        
        // If it doesn't match anything just consume it
        cc = getchar();

    } while(1);

// Cerain token (like symbols) keep reading until an invalid character is found
// so they will jump in lexerReturn (cc is already outside of token)
// While other (like parenthesis and strings) are single-character or have
// well-defined begin-end, so they don't need to consume an additional character
// to ensure both work the same way, the latter kind will jump to
// lexerConsumeAndReturn
lexerConsumeAndReturn:
    cc = getchar();
lexerReturn:
    putch('\0');
    return;
}

// LISP simplified BNF
//
// S -> A | (L)                 <-- readForm
// L -> \epsilon | SL           <-- readList

void readForm(BoxRef boxRef);
void readList(BoxRef boxRef);

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
        if (token.type != TTYPE_RPAR) {
            *boxRef = boxSignal(SIGNAL_SYNTAX_ERROR);
        } else if (getTag(boxRef) == TAG_SIGNAL) {
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
            setValue(boxRef, (Value)rawRef);
            // Should check that the returned tag is not a signal, but since this
            // call relies under TTYPE_STRING/SYMBOL/LABEL there is no need to check
            setTag(boxRef, ttypeToTag(token.type));
        }
        // Otherwise keep signal
        break;
    case TTYPE_DOT:
        // If a dot appears here then a list is malformed
        *boxRef = boxSignal(SIGNAL_SYNTAX_ERROR);
    case TTYPE_RPAR:
        // A form is either an atomic type or the beginning of a list
        *boxRef = boxSignal(SIGNAL_SYNTAX_ERROR);
        break; 
    default:
        todo("Support all symbols");
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

        // Temporary value to be added in a car
    Box value = boxNil(),
        // Reference to the previous cons, temporary reference to list's tail
        prev  = boxNil();
    pointerRegistryPush(&value);
    pointerRegistryPush(&prev);

    // If first token is a right parenthesis, empty list evaluates to nil,
    // skip list creation alltogether
    if (token.type == TTYPE_RPAR) {
        goto readListEnd;
    }
    
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
        setValue(&prevConsRef->cdr, (Value) consRef);
        setTag(&prevConsRef->cdr,   (Tag) TAG_CONS);

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

// Using a label so the code is not as messy, could make a dedicated end
// function which pops the desired number of pointers and recycle it for other
// functions
readListEnd:

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
