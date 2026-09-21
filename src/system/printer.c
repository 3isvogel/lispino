#include "printer.h"
#include "utility/log.h"

#include <utility/box.h>
#include <utility/signals.h>
#include <utility/log.h>

#include <memory/heap.h>

#include <stdio.h>

void innerPrint(Box box, int readable) {
    switch(getTag(&box)) {
    case TAG_CONS:
        printf("(");
        Cons* cons = (Cons*)getValue(&box);
        // Print the car
        innerPrint(cons->car, readable);
        // As long as a cons exists to the right, follow it and repeat printing
        while(getTag(&cons->cdr) == TAG_CONS) {
            cons = (Cons*)getValue(&cons->cdr);
            printf(" "); innerPrint(cons->car, readable);
        }

        // At the end of a list, if cons is not nil print it separated by a "."
        if (getTag(&cons->cdr) != TAG_NIL) {
            printf(" . ");
            innerPrint(cons->cdr, readable);
        }

        // Then close the list with a ")"
        printf(")");
        break;
    case TAG_STRING:
        if (readable)   printf("%s", getRaw(box));
        else            printf("\"%s\"", getRaw(box));
        break;
    case TAG_LABEL:
        printf(":%s", getRaw(box));
        break;
    case TAG_SYMBOL:
        // Get the raw content of a string type
        printf("%s", getRaw(box));
        break;
    case TAG_SIGNAL:
        if (readable) printf("SIG#%d", (int)getValue(&box));
        break;
    case TAG_NIL:
        if (!readable) printf("nil");
        break;
    case TAG_PRIMITIVE:
        if (!readable) printf("<pri@%02x>", (unsigned int)getValue(&box));
        break;
    case TAG_CLOSURE:
        printf("[(lambda ");
        // If a closure exists I know it's well formed
        box = getCar(&box);
        if(getTag(&box) == TAG_CONS) {
            innerPrint(getCar(&box), readable);
            box = getCdr(&box);
        }
        while (getTag(&box) == TAG_CONS) {
            printf(" ");
            innerPrint(getCar(&box), readable);
            box = getCdr(&box);
        }
        printf(")]");
        break;
    default:
        printf("%d", (int) getValue(&box));
        break;
    }
}

void Print(Box box) {
    // This code is called after evaluation, if all references were valid they
    // still are here, memory is never allocated and gc will never perform any
    // moving here, so it's safe to work without registering any pointer
    if(getTag(&box) == TAG_SIGNAL) {
        fprintf(stderr, "; " TERM_CODE_SET2(TERM_CODE_BOLD, TERM_CODE_FG(TERM_COLOR_RED)) "SIGNAL %2d" TERM_CODE_RESET ": %s", (Signal)getValue(&box), strSignal(getValue(&box)));
    }
    innerPrint(box, 0);
    // Newline (and flush)
    printf("\n");
}
