#include <stdlib.h>
#include "errors.h"
#include "log.h"
#include "mem.h"

#define X(x,s) #x,
char* printableSignals[SIGNAL_SIZE] = {
SIGNAL_LIST
};
#undef X

void todo(const char* const message) {
    logError("%s", message);
    fail(SIGNAL_TO_DO);
}

void fail(Signal signal) {
    logError("Failing with signal %2d: %s", signal, printableSignals[signal]);
    // Clean all allocated memory
    del_memory();
    exit(signal);
}

char* strSignal(Signal signal) {
    return printableSignals[signal];
}
