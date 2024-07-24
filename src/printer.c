#include <stdio.h>
#include "log.h"
#include "printer.h"
#include "errors.h"
#include "heap.h"
#include "types.h"

#define PRINT_BUFFER_LEN 1024

#define CEL(x) ((Cell)x)

// alernatively print on buffer
char print_buffer[PRINT_BUFFER_LEN];

void line_print(Box ast);

void Print(Box ast) {
    line_print(ast);
    printf("\n");
    C_GC();
}

void line_print(Box ast) {
    Cell val = CELL(ast);
    char* format;
    switch(get_tag(ast)) {
        case NIL:
            printf("NIL");
            break;
        case INT:
            printf("%ld", (long)val);
            break;
        case SYM:
            printf("%s", raw_adr(val));
            break;
        case LAB:
            printf("%s", raw_adr(val));
            break;
        case STR:
            printf("\"%s\"", raw_adr(val));
            break;
        case F64:
            printf("%f", ast);
            return;
        case ERR:;
            printf("ERR[%s]", ERS[LONG(val)]);
            return;
        case CON:
            // TODO for now cons cannot be created nor printed
            printf("(");
            do {
                ast = val->cdr;
                line_print(val->car);
                val = CELL(ast);
            } while(get_tag(ast) == CON && printf(" "));
            if(get_tag(ast) != NIL) {
                printf(" . ");
                line_print(ast);
            }
            printf(")");
            break;
        case PRI:
            printf("<prim@%lx>", LONG(val));
            break;
        case CLO:
            printf("<clos@%lx", LONG(val));
            ast = val->car;
            while(get_tag(ast) == CON) {
                val = CELL(ast);
                printf(" ");
                line_print(val->car);
                ast = val->cdr;
            }
            printf(">");
            break;
        default:
            logError("uhhh...");
            fail(UNEXPECTED_BRANCH);
    }
    return;
}
