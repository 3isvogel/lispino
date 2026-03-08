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
            todo("Implement string parsing");
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
    Box car, cdr, box;
    pointerRegistryPush(&box);
    pointerRegistryPush(&car);
    pointerRegistryPush(&cdr);

    if (token.type == TTYPE_RPAR) {
        box = setBox(0, TAG_NIL);
    } else {

        // TODO: in this whole function GC can only be called here, reduce scope
        //       if TCO does not prevent me from doing so {
            car = readForm();
            cdr = readList();

            // consRef is not a registered pointer, but this not a problem,
            // this reference is only used temporary to construct the cons,
            // once the cons is assigned to "box", the consRef might be
            // invalidated by GC, but the cons itself will survive being
            // referenced by a registered pointer "box"
            Cons* consRef = newCons();
        // }

        consRef->car = car;
        consRef->cdr = cdr;

        box = setBox((Value) consRef, TAG_CONS);
    }

    pointerRegistryPop();
    pointerRegistryPop();
    pointerRegistryPop();
    return box;
}

Box Read() {
    return readForm();
}
