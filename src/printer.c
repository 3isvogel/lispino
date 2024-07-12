#include <stdio.h>
#include "log.h"
#include "printer.h"
#include "types.h"

#define PRINT_BUFFER_LEN 1024

#define CEL(x) ((Cell)x)

// alernatively print on buffer
char print_buffer[PRINT_BUFFER_LEN];

void line_print(Box ast);

void Print(Box ast) {
    line_print(ast);
    printf("\n");
}

void line_print(Box ast) {
    Cell val = (Cell)get_val(ast);
    switch(get_tag(ast)) {
        case NIL:
            printf("NIL");
            break;
        case INT:
            printf("%ld", (long)val);
            break;
        case SYM:
            printf("%s", (char*)val);
            break;
        case LAB:
            printf("%s", (char*)val);
            break;
        case STR:
            printf("\"%s\"", (char*)val);
            break;
        case F64:
            printf("%f", ast);
            return;
        case CON:
            // TODO for now cons cannot be created nor printed
            printf("(");
            do {
                ast = val->cdr;
                line_print(val->car);
                val = (Cell)get_val(ast);
            } while(get_tag(ast) == CON && printf(" "));
            if(get_tag(ast) != NIL) {
                printf(" . ");
                line_print(ast);
            }
            printf(")");
            break;
        default:
            printf("Uhhh...");
    }
    return;
}
