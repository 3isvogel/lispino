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
    long val = get_val(ast);
    Cell curr;
    switch(get_tag(ast)) {
        case NIL:
            printf("NIL");
            break;
        case INT:
            printf("%d", (int)val);
            break;
        case SYM:
            printf("%s", (char*)val);
            break;
        case LABL:
            printf("%s", (char*)val);
            break;
        case STRI:
            printf("\"%s\"", (char*)val);
            break;
        case DOUB:
            printf("%f", ast);
            return;
        case CONS:
            // TODO for now cons cannot be created nor printed
            printf("(");
            do {
                curr = (Cell)val;
                line_print(curr->car);
                if(get_tag(curr->cdr) != NIL)
                    printf(" ");
                else break;
                val = get_val(curr->cdr);
            } while(1);
            printf(")");
            break;
        default:
            printf("Uhhh...");
    }
    return;
}
