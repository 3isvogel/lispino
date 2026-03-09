#include <utility/signals.h>
#include <utility/log.h>

#include <memory/mem.h>
#include <memory/stack.h>
#include <memory/heap.h>

#include <system/parser.h>
#include <system/eval.h>
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
        totalSize;
    tokenBufferSize = (TOKEN_BUFFER_MAX_LEN+1) * sizeof(char);
    stackSize = (STACK_MAX_LEN) * sizeof(Cons);
    heapSize = (HEAP_MAX_LEN) * sizeof(Box) * 2;
    pointerRegistrySize = (POINTER_REGISTRY_MAX_LEN) * sizeof(Box*);

    // Using two heaps: copy GC
    totalSize = tokenBufferSize + stackSize + heapSize + pointerRegistrySize;

    logInfo("Memory         size size");
    logInfo("Token buffer:  %5d (%dK)", tokenBufferSize, tokenBufferSize/K);
    logInfo("Symbols stack: %5d (%dK)", stackSize, stackSize/K);
    logInfo("Heaps:         %5d (%dK)", heapSize, (heapSize/K));
    logInfo("Symbols stack: %5d (%dK)", pointerRegistrySize, pointerRegistrySize/K);
    logInfo("Total:         %5d (%dK)", totalSize, totalSize/K);
}

int main(int argc, char** argv) {

    logSetLevel(LOG_LEVEL_DEBUG);

    if (createMemory() == 0) {
        fail(SIGNAL_MEM_SETUP_FAIL);
    }
    printsize();
    // if(!env_init()) fail(ENV_INIT_FAIL);

    while(1) {
        printf("%d > ", heapAvailableSize());
        // flush for when using pipes 
        fflush(stdout);
        // Read 1 vaild s-expr
        Box ret = Read();
        ret = Eval(ret);
        Print(ret);
    }
}
