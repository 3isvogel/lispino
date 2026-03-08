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
            putch('(');
            goto lexerConsumeAndReturn;
        } else if (cc == ')') {
            token.type = TTYPE_RPAR;
            putch(')');
            goto lexerConsumeAndReturn;
        //} else if (cc == '.') {
        //    token.type = TTYPE_DOT;
        //    return;
        //} else if (cc == '\'') {
        //    token.type = TTYPE_QUOTE;
        //    return;
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

Box readForm();
Box readList();

Box readForm() {
    // Save in pointer registry for automatic update on GC
    // Every registered Box MUST be initialized to prevent unwanted behavior
    Box box = nilBox();
    BoxRef rawRef;
    pointerRegistryPush(&box);

    switch(token.type) {
    case TTYPE_LPAR:
        // call readList
        next();
        box = readList();
        break;
    case TTYPE_DOT:
        todo("Implement dot syntax");
    case TTYPE_RPAR:
        // A form is either an atomic type or the beginning of a list
        box = boxSignal(SIGNAL_SYNTAX_ERROR);
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

        box = setRaw(rawRef, token.text);
        // If assignment didn't fail populate box with value and tag
        if (getTag(&box) != TAG_SIGNAL) {
            setValue(&box, (Value)rawRef);
            // Should check that the returned tag is not a signal, but since this
            // call relies under TTYPE_STRING/SYMBOL/LABEL there is no need to check
            setTag(&box, ttypeToTag(token.type));
        }
        // Otherwise keep signal
        break;
    default:
        todo("Support all symbols");
    }

    pointerRegistryPop();
    return box;
}

Box readList() {

    // TODO: can reduce the scope of pointer registered to just readForm and
    //       readList
    Box car = nilBox(),
        cdr = nilBox(),
        box = nilBox();
    pointerRegistryPush(&box);
    pointerRegistryPush(&car);
    pointerRegistryPush(&cdr);

    // If TTYPE_RPAR return empty list (which is nil)
    if (token.type != TTYPE_RPAR) {
        // TODO: in this whole function GC can only be called here, reduce scope
        //       if TCO does not prevent me from doing so {
            car = readForm();
            next();
            cdr = readList();

            // consRef is not a registered pointer, but this not a problem,
            // this reference is only used temporary to construct the cons,
            // once the cons is assigned to "box", the consRef might be
            // invalidated by GC, but the cons itself will survive being
            // referenced by a registered pointer "box"
            Cons* consRef = newCons();
        // }

        if (token.type == TTYPE_RPAR) {
            consRef->car = car;
            consRef->cdr = cdr;

            box = setBox((Value) consRef, TAG_CONS);
        } else {
            box = boxSignal(SIGNAL_SYNTAX_ERROR);
        }
    }

    pointerRegistryPop();
    pointerRegistryPop();
    pointerRegistryPop();
    return box;
}

Box Read() {
    next();
    return readForm();
}
