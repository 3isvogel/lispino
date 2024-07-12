#include "reader.h"
#include "errors.h"
#include "heap.h"
#include "log.h"
#include "types.h"
#include <stdio.h>
#include <string.h>

char token_buffer[(TOKENBUF_MAX_LEN + 1)], see = ' ';
unsigned int token_len;
int depth = 0;

#define CH0 token_buffer[0]

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

void get_token() {
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
        break;
    default:
        do
          token_buffer[i++] = get();
        while (i < TOKENBUF_MAX_LEN && !curr(' ') && !special());
    }
    token_buffer[i] = '\0';
    token_len = i;
    return;
}

Box read_list();
Box read_atom();

void close_form() {
    if (depth > 0) get_token();
}

Box read_form() {
    Box t;
    switch (CH0) {
    case '(':
        depth++;
        get_token();
        t = read_list();
        break;
    case ')':
        depth --;
        break;
    default:
        t = read_atom();
    }
    close_form();
    if(depth < 0) fail(UNBALANCED);
    return t;
}

Box Read() {
    get_token();
    return read_form();
}

#define cons_debug(name)\
logDebug("  %12lx [%s]  | %12lx [%s]", get_val(name->car), type_name[get_tag(name->car)], get_val(name->cdr), type_name[get_tag(name->cdr)])

Box read_list() {
    if (CH0 == ')') {
        depth --;
        return nil;
    }
    Cell head = get_mem(sizeof(Cell_t));
    head->car = read_form();

    Cell cons = head;
    while (CH0 != ')' && !(CH0 == '.' && token_len == 1)) {
        Cell t = (Cell)get_mem(sizeof(Cell_t));
        cons->cdr = box(CON, LONG(t));
        cons_debug(cons);
        cons = (Cell)get_val(cons->cdr);
        cons->car = read_form();
    }
    if(CH0 == '.' && token_len == 1) {
        get_token();
        cons->cdr = read_form();
    } else {
        cons->cdr = nil;
    }
    cons_debug(cons);
    depth --;
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

Box read_atom() {
    if (!strncmp(token_buffer, "nil", TOKENBUF_MAX_LEN)) {
        return nil;
    }

    switch (CH0) {
    case ':':
        // Request memory, populate it, box its address and return it
        return box(LAB, (long)memcpy(get_mem(token_len + 1), token_buffer, token_len + 1));
    case '"':
        return box(STR, (long)memcpy(get_mem(token_len), &token_buffer[1], token_len));
    default:
        switch (numsym()) {
        case numSym_sym:
            return box(SYM, (long)memcpy(get_mem(token_len + 1), token_buffer, token_len + 1));
        case numSym_int:
            return box(INT, (long)atoi(token_buffer));
        case numSym_double:
            return strtod(token_buffer, NULL);
        }
    }
    caccamerda();
    return -1;
}
