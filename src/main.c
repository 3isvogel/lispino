#include <utility/log.h>
#include <utility/signals.h>

#include <memory/mem.h>
#include <memory/heap.h>
#include <memory/stack.h>

#include <system/eval.h>
#include <system/parser.h>
#include <system/printer.h>

#include <stdio.h>

// FIXME: problems with GC, it changes address of everything, so either
// - add ANOTHER stack in which to store vars in boxed form and update pointers
//   when gc() is called and only point to them from the code, it may be good
//   if (use actual vars like would normally do, push value on stack right
//   before the call that might move it, pop it right after
// - use a specific pattern for gc()
// - consider invalid any value after a function call that may allocate values,
//   re-solve symbols
// - add Box parameter to gc() -> gc(Box) and treat it as an additional root:
//   do not collect data attached to it and update it (or return its new value)
//   to keep it valid through gc
//
// - Register pointers before any GC can happen and pop it (needs another stack)
//   keep the rest of the code the same

#define K 1024

void printsize() {
    int tokenBufferSize,
        stackSize,
        heapSize,
        pointerRegistrySize,
        rawMapSize,
        totalSize;
    tokenBufferSize = (TOKEN_BUFFER_MAX_LEN+1) * sizeof(char);
    stackSize = (STACK_MAX_LEN) * sizeof(Cons);
    heapSize = (HEAP_MAX_LEN) * sizeof(Box) * 2;
    rawMapSize = getRawStringMapSize() * sizeof(BoxRef);
    pointerRegistrySize = (POINTER_REGISTRY_MAX_LEN) * sizeof(Box*);

    // Using two heaps: copy GC
    totalSize = tokenBufferSize + stackSize + heapSize + pointerRegistrySize + rawMapSize;

    logInfo("%18s %10s %10s", "Memory", "size (B)", "size (kB)");
    logInfo("%18s %10d %10d", "Token buffer:", tokenBufferSize, tokenBufferSize/K);
    logInfo("%18s %10d %10d", "Symbols stack:", stackSize, stackSize/K);
    logInfo("%18s %10d %10d", "Heaps (x2):", heapSize, (heapSize/K));
    logInfo("%18s %10d %10d", "Pointer registry:", pointerRegistrySize, pointerRegistrySize/K);
    logInfo("%18s %10d %10d", "Raw String Map:", rawMapSize, rawMapSize/K);
    logInfo("%18s %10d %10d", "Total:", totalSize, totalSize/K);
    logInfo("");
    logInfo("%18s %10d", "Box:", sizeof(Box));
    logInfo("%18s %10d", "Cons cell:", sizeof(Cons));
}

int main(int argc, char** argv) {

    logSetLevel(LOG_LEVEL_DEBUG);

    if (createMemory() == 0) {
        fail(SIGNAL_MEM_SETUP_FAIL);
    }

    printsize();
    // if(!env_init()) fail(ENV_INIT_FAIL);
    initializeEnv();

    while(1) {
        printf("%d > ", heapAvailableSize());
        // flush for when using pipes 
        fflush(stdout);
        // Read 1 vaild s-expr
        Box ret = Read();
        // TODO: remove temporary leaky check
        if (pointerRegistryLeaking()) fail(SIGNAL_POINTER_REGISTRY_LEAKING);
        ret = Eval(ret);
        // TODO: remove temporary leaky check
        if (pointerRegistryLeaking()) fail(SIGNAL_POINTER_REGISTRY_LEAKING);
        Print(ret);
    }
}
