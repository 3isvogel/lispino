#include <stdlib.h>
#include "errors.h"
#include "types.h"
#include "log.h"
#include "mem.h"

#define X(x) #x,
char* ERS[ERR_MAX] = {
ERROR_LIST
};
#undef X

int fail(int e) {
    if(e == EOF_REACHED)
        logWarning("Closing: %s", ERS[EOF_REACHED]);
    else
        logError("Closing: %s", ERS[e]);
    del_memory();
    exit(e);
}

Box err(int e) {
    return box(ERR, e);
}
