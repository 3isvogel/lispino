#include "printer.h"
#include "utility/log.h"

#include <utility/box.h>
#include <utility/signals.h>
#include <utility/log.h>

#include <memory/heap.h>

#include <stdio.h>

void innerPrint(Box box) {
    switch(getTag(&box)) {
    case TAG_CONS:
        printf("(");
        Cons* cons = (Cons*)getValue(&box);
        // Print the car
        innerPrint(cons->car);
        // As long as a cons exists to the right, follow it and repeat printing
        while(getTag(&cons->cdr) == TAG_CONS) {
            cons = (Cons*)getValue(&cons->cdr);
            printf(" "); innerPrint(cons->car);
        }

        // At the end of a list, if cons is not nil print it separated by a "."
        if (getTag(&cons->cdr) != TAG_NIL) {
            printf(" . ");
            innerPrint(cons->cdr);
        }

        // Then close the list with a ")"
        printf(")");
        break;
    case TAG_STRING:
        printf("\"%s\"", getRaw(box));
        break;
    case TAG_SYMBOL:
    case TAG_LABEL:
        // Get the raw content of a string type
        printf("%s", getRaw(box));
        break;
    case TAG_SIGNAL:
        fprintf(stderr, ";" " " SET2E(BOLD_CODE, FG(RED_CODE)) "SIGNAL %2d" RESET ": %s", (Signal)getValue(&box), strSignal(getValue(&box)));
        break;
    case TAG_NIL:
        printf("nil");
        break;
    case TAG_PRIMITIVE:
        printf("<pri@%p>", (void*)getValue(&box));
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
    innerPrint(box);
    // Newline (and flush)
    printf("\n");
}
