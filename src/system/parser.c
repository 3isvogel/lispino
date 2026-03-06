#include <utility/signals.h>
#include <memory/mem.h>
#include "box.h"
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
 * @return pointer to allocated buffer, used by memory management to free it later
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
    
        // Special characters, used as abbreviations for longer forms, returns just a single character
        } else if (cc == '(') {
            token.type = TTYPE_LPAR;
            return;
        } else if (cc == ')') {
            token.type = TTYPE_RPAR;
            return;
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
            } while(cc >= '*' && cc <= '~');
            token.type = TTYPE_SYMBOL;
            return;
        } else if (cc >= '!' && cc <= '\'') {
            todo("Implement special characters");
        }

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

Box* Read() {
    
}

#define cons_debug(name)\
logDebug("  %12lx [%s]  | %12lx [%s]", get_val(name->car), type_name[get_tag(name->car)], get_val(name->cdr), type_name[get_tag(name->cdr)])

Box read_list() {
    get_token();
    if (CH0 == ')') {
        return box(NIL, 1);
    }
    Cell head = get_mem(sizeof(Cell_t));
    head->car = read_form();
    get_token();

    Cell cons = head;
    while (CH0 != ')' && !(CH0 == '.' && token_len == 1)) {
        Cell t = (Cell)get_mem(sizeof(Cell_t));
        cons->cdr = box(CON, LONG(t));
        cons_debug(cons);
        cons = (Cell)get_val(cons->cdr);
        cons->car = read_form();
        get_token();
    }
    if(CH0 == '.' && token_len == 1) {
        get_token();
        cons->cdr = read_form();
        get_token();
    } else {
        cons->cdr = nil;
    }
    if(CH0 != ')') return box(ERR, UNBALANCED);
    cons_debug(cons);
    return box(CON, LONG(head));
}

int numsym() {
    int i = 0;
    int floaty = 0;
    if (token_buffer[i] == '+' || token_buffer[i] == '-') {
        if(token_len == 1) return 0;
        i = 1;
    }
    for (; i < token_len; i++) {
        if (token_buffer[i] < '0' || token_buffer[i] > '9') {
            if ((!floaty) && token_buffer[i] == '.')
                floaty = 1;
            else
                return 0;
        }
    }
    return 1 + floaty;
}

enum { numSym_sym, numSym_int, numSym_double };

Box quote_exp() {
    Cell quote = get_mem(sizeof(Cell_t)),
         args  = get_mem(sizeof(Cell_t));
    quote->car = box(SYM, (long)memcpy((char*)raw_mem(6) + 2, "quote", 6) - 2);
    quote->cdr = box(CON, (long)args);
    args->cdr = box(NIL, 0);
    get_token();
    args->car = read_form();
    return box(CON, (long)quote);
}

Box read_atom() {
    if (!strncmp(token_buffer, "nil", TOKENBUF_MAX_LEN)) {
        return nil;
    }

    switch (CH0) {
    case '\'':
        return quote_exp();
    case ':':
        return box(LAB, (long)memcpy(((char*)raw_mem(token_len + 1)) + 2, (Cell)token_buffer, token_len + 1) - 2);
    case '"':
        return box(STR, (long)memcpy(((char*)raw_mem(token_len) + 2), (Cell)&token_buffer[1], token_len) - 2);
    }
    switch (numsym()) {
    case numSym_sym:
        return box(SYM, (long)memcpy(((char*)raw_mem(token_len + 1) + 2), (Cell)token_buffer, token_len + 1) - 2);
    case numSym_int:
        return box(INT, (long)atoi(token_buffer));
    case numSym_double:
        return strtod(token_buffer, NULL);
    }
    return fail(0);
}
