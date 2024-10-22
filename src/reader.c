#include "reader.h"
#include "errors.h"
#include "heap.h"
#include "log.h"
#include "types.h"
#include <stdio.h>
#include <string.h>

char* token_buffer, see = ' ';
unsigned int token_len;

#define CH0 token_buffer[0]

char* init_reader(unsigned size) {
    token_buffer = (char*)malloc(sizeof(char) * (size + 1));
    logAlloc("Allocating tokenbuffer at %p", token_buffer);
    return token_buffer;
}
void del_reader() { free(token_buffer); }

int curr(char c) {
    if (c == ' ') {
        return see > 0 && see <= c; // match any whitespace
    } else {
        return see == c; // match specific char
    }
}

int special() {
    return see == ')' || see == '(' || see == '\'' || see == ';' || see == '"';
}

int peek() { return see; }

char get() {
    char c = see;
    see = getchar();
    if (see == EOF)
        fail(EOF_REACHED);
    return c;
}

Box get_token() {
    int i = 0;
    while (curr(' '))
        get();
    if (curr(';')) {
        while (!curr('\n')) get(); // skip comments
        get();
    }
    switch (see) {
    case '\'': case '(': case ')':
        token_buffer[i++] = get();
        break;
    case '"':
        do {
            token_buffer[i++] = get();
            if(curr('"')) {get(); goto finalize;}
            while (curr('\\') && i < TOKENBUF_MAX_LEN) {
                get();
                // https://github.com/Robert-van-Engelen/lisp-cheney/blob/main/src/lisp-cheney.c
                // smart way to compact escape characters:
                // escape chars start from 7 to ...
                static const char *escs = "abtnvfr";
                const char *p = strchr(escs, see);
                token_buffer[i++] = p ? 7 + p - escs : see;
                get();
            }
        } while (!curr('"') && !curr('\n') && i < TOKENBUF_MAX_LEN);
        if (get() != '"')
            fail(UNTERMINATED_STR);
        break;
    default:
        do
          token_buffer[i++] = get();
        while (i < TOKENBUF_MAX_LEN && !curr(' ') && !special());
    }
finalize:
    token_buffer[i] = '\0';
    token_len = i;
    return nil;
}

Box read_list();
Box read_atom();

Box read_form() {
    Box t;
    switch (CH0) {
    case '(':
        t = read_list();
        break;
    case ')':
        t = box(ERR, UNBALANCED);
        break;
    default:
        t = read_atom();
    }
    return t;
}

Box Read() {
    get_token();
    Box t = read_form();
    return t;
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
