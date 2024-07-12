#include <stdlib.h>
#include "errors.h"
#include "log.h"

char* ERR[] = {
    "EOF reached",
    "not enough memory available",
    "unbalanced parenthesis",
    "unterminated string, you either forgot to close a \" or the string is too long"
};

void fail(int e) {
    if(e)
        logError("Closing: %s", ERR[e]);
    else
        logWarning("Closing: %s", ERR[0]);
    exit(e);
}

void caccamerda() {
    logError("CACCAMERDA");
    exit(-1);
}
