#include "printer.h"
#include "utility/log.h"

#include <utility/box.h>
#include <utility/signals.h>
#include <utility/log.h>

#include <memory/heap.h>

#include <stdio.h>

void innerPrint(Box box, Readable readable, FILE* file) {
    switch(getTag(&box)) {
    case TAG_CONS:
        fprintf(file, "(");
        Cons* cons = (Cons*)getValue(&box);
        // Print the car
        innerPrint(cons->car, readable, file);
        // As long as a cons exists to the right, follow it and repeat printing
        while(getTag(&cons->cdr) == TAG_CONS) {
            cons = (Cons*)getValue(&cons->cdr);
            fprintf(file, " "); innerPrint(cons->car, readable, file);
        }

        // At the end of a list, if cons is not nil print it separated by a "."
        if (getTag(&cons->cdr) != TAG_NIL) {
            fprintf(file, " . ");
            innerPrint(cons->cdr, readable, file);
        }

        // Then close the list with a ")"
        fprintf(file, ")");
        break;
    case TAG_STRING:
        if (readable)   fprintf(file, "%s", getRaw(box));
        else            fprintf(file, "\"%s\"", getRaw(box));
        break;
    case TAG_LABEL:
        fprintf(file, ":%s", getRaw(box));
        break;
    case TAG_SYMBOL:
        // Get the raw content of a string type
        fprintf(file, "%s", getRaw(box));
        break;
    case TAG_SIGNAL:
        if (readable) fprintf(file, "SIGNAL<%s>", strSignal(getValue(&box)));
        break;
    case TAG_NIL:
        if (!readable) fprintf(file, "nil");
        break;
    case TAG_PRIMITIVE:
        if (!readable) fprintf(file, "<pri@%02x>", (unsigned int)getValue(&box));
        break;
    case TAG_CLOSURE:
        fprintf(file, "[(lambda ");
        // If a closure exists I know it's well formed
        box = getCar(&box);
        if(getTag(&box) == TAG_CONS) {
            innerPrint(getCar(&box), readable, file);
            box = getCdr(&box);
        }
        while (getTag(&box) == TAG_CONS) {
            fprintf(file, " ");
            innerPrint(getCar(&box), readable, file);
            box = getCdr(&box);
        }
        fprintf(file, ")]");
        break;
    default:
        fprintf(file, "%lld", getValue(&box));
        break;
    }
}

void Print(Box box, FILE *file) {
    // This code is called after evaluation, if all references were valid they
    // still are here, memory is never allocated and gc will never perform any
    // moving here, so it's safe to work without registering any pointer
    if(getTag(&box) == TAG_SIGNAL) {
        fprintf(stderr, "; " TERM_CODE_SET2(TERM_CODE_BOLD, TERM_CODE_FG(TERM_COLOR_RED)) "SIGNAL %2d" TERM_CODE_RESET ": %s", (Signal)getValue(&box), strSignal(getValue(&box)));
    }
    innerPrint(box, 0, file);
    // Newline (and flush)
    printf("\n");
}
