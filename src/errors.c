#include <stdlib.h>
#include "errors.h"
#include "types.h"
#include "log.h"

#define X(x) #x,
char* ERS[ERR_MAX] = {
ERROR_LIST
};
#undef X

void fail(int e) {
    if(e)
        logError("Closing: %s", ERS[e]);
    else
        logWarning("Closing: %s", ERS[0]);
    exit(e);
}

Box err(int e) {
    return box(ERR, e);
}

void caccamerda() {
    logError("CACCAMERDA");
    exit(-1);
}
