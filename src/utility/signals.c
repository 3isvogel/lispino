#include "signals.h"
#include "log.h"
#include "box.h"
#include <memory/mem.h>

#define X(x,s) #x,
char* printableSignals[SIGNAL_SIZE] = {
SIGNAL_LIST
};
#undef X

void _lineFail(Signal signal, char* file, int line) {
    // needs explicit function to pass file and line to function
    logPrint(LOG_LEVEL_ERROR, file, line, "Fail with signal %2d: %s", signal, strSignal(signal));
    // Clean all allocated memory
    destroyMemory();
    exit(signal);
}

Box boxSignal(Signal signal) {
    return setBox((Value)signal, TAG_SIGNAL);
}

char* strSignal(Signal signal) {
    if (signal >= SIGNAL_SIZE)
        return printableSignals[SIGNAL_UNKNOWN_FAILURE];
    return printableSignals[signal];
}

unsigned int signalPass(BoxRef dest, BoxRef src) {
    if (getTag(src) != TAG_SIGNAL) return 0;
   *dest = *src;
   return 1;
}
