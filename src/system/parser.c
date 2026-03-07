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
X(LPAR)             \
X(RPAR)             \
X(STRING)           \
X(SYMBOL)           \
X(DOT)              \
X(NIL)              \
X(QUOTE)

#define X(x) TTYPE_##x,
typedef enum {
    TOKEN_TYPE_LIST \
    TOKEN_TYPE_SIZE
} TokenType;
#undef X

typedef long long int TokenValue;

typedef struct TokenBuffer_s {
    char*           text;
    unsigned int    len;
    TokenType       type;
    TokenValue      value;
    unsigned int    bufferSize;
} TokenBuffer;
TokenBuffer token;

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

    token = (struct TokenBuffer_s) {
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
        fail(SIGNAL_TOKEN_BUFFER_FULL);
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
int cc;
void next() {

    clearToken();

    do {
        cc = getchar();
    
        if (cc == EOF) {
            // TODO: actually handle this
            fail(SIGNAL_EOF_REACHED);
    
        // TODO: handle special characters, used as abbreviations for longer
        // forms, returns
        // just a single character
        } else if (cc == '(') {
            token.type = TTYPE_LPAR;
            break;
        } else if (cc == ')') {
            token.type = TTYPE_RPAR;
            break;
        //} else if (cc == '.') {
        //    token.type = TTYPE_DOT;
        //    return;
        //} else if (cc == '\'') {
        //    token.type = TTYPE_QUOTE;
        //    return;
        } else if (cc ==  '"') {
            todo("Implement string parsing");
            // Keep adding character until a non-escaped " is found
        } else if (cc ==  ';') {
            while((cc = getchar()) != '\n') {};
        // TODO: recognize symbols
        } else if (cc >= '*' && cc <= '~') {
            do {
                putch(cc);
                cc = getchar();
            } while(cc >= '*' && cc <= '~');
            token.text[token.len ++ ] = '\0';
            token.type = TTYPE_SYMBOL;
            break;
        } else if (cc >= '!' && cc <= '\'') {
            todo("Implement special characters");
        }

    logDebug("Got token: %s", token.text);

    } while(1);
}

// // Keep to later escape string 
// do {
//     token_buffer[i++] = get();
//     if(curr('"')) {get(); goto finalize;}
//     while (curr('\\') && i < TOKENBUF_MAX_LEN) {
//         get();
//         // https://github.com/Robert-van-Engelen/lisp-cheney/blob/main/src/lisp-cheney.c
//         // smart way to compact escape characters:
//         // escape chars start from 7 to ...
//         static const char *escs = "abtnvfr";
//         const char *p = strchr(escs, see);
//         token_buffer[i++] = p ? 7 + p - escs : see;
//         get();
//     }
// } while (!curr('"') && !curr('\n') && i < TOKENBUF_MAX_LEN);
// if (get() != '"')
//     fail(UNTERMINATED_STR);

// LISP simplified BNF
//
// S -> A | (L)                 <-- readForm
// L -> \epsilon | SL           <-- readList

Box readForm();
Box readList();

Box readForm() {
    // Consume next token
    next();

    // Save in pointer registry for automatic update on GC
    Box box;
    BoxRef rawRef;
    pointerRegistryPush(&box);

    switch(token.type) {
    case TTYPE_LPAR:
        // call readList
        box = readList();
        break;
    case TTYPE_RPAR:
    case TTYPE_DOT:
        // A form is either an atomic type or the beginning of a list
        box = boxSignal(SIGNAL_SYNTAX_ERROR);
        break;
    // read atomic
    case TTYPE_SYMBOL:
        // Request new memory and fill with value
        rawRef = newRaw(token.len);
        box = setRaw(rawRef, token.text);
        // If value does not fit: return the boxed signal
        if (getValue(&box) == TAG_SIGNAL)
            break;
        // Else the value is successfully inserted into the heap
        setTag(&box, TAG_SYMBOL);
        setValue(&box, (Value)rawRef);
    default:
        todo("Support all symbols");
    }

    pointerRegistryPop();
    return box;
}

Box readList() {

    // Consume next token
    next();

    Box car, cdr, box;
    pointerRegistryPush(&box);
    pointerRegistryPush(&car);
    pointerRegistryPush(&cdr);

    if (token.type == TTYPE_RPAR) {
        car = setBox(0, TAG_NIL);
    } else {
        car = readForm();
        cdr = readList();
    }

    Cons* consRef = newCons();
    consRef->car = car;
    consRef->cdr = cdr;
    box = setBox((Value) consRef, TAG_CONS);

    pointerRegistryPop();
    pointerRegistryPop();
    pointerRegistryPop();
    return box;
}

Box Read() {
    return readForm();
}
