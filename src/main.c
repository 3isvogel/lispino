#include <utility/log.h>
#include <utility/signals.h>

#include <memory/mem.h>
#include <memory/heap.h>
#include <memory/stack.h>

#include <system/eval.h>
#include <system/parser.h>
#include <system/printer.h>

#include <stdio.h>

#define K (1024)

void printsize() {
    unsigned int tokenBufferSize,
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

    logInfo("%18s %10s %10s", "Memory", "size (B)", "size (KB)");
    logInfo("%18s %10u %10u", "Token buffer:",      tokenBufferSize, tokenBufferSize/K);
    logInfo("%18s %10u %10u", "Symbols stack:",     stackSize, stackSize/K);
    logInfo("%18s %10u %10u", "Heaps (x2):",        heapSize, (heapSize/K));
    logInfo("%18s %10u %10u", "Pointer registry:",  pointerRegistrySize, pointerRegistrySize/K);
    logInfo("%18s %10u %10u", "Raw String Map:",    rawMapSize, rawMapSize/K);
    logInfo("%18s %10u %10u", "Total:",             totalSize, totalSize/K);
    logInfo("");
    logInfo("%18s %10u", "Box:", sizeof(Box));
    logInfo("%18s %10u", "Cons cell:", sizeof(Cons));
}

int main(int argc, char** argv) {

    logSetLevel(LOG_LEVEL_DEBUG);

    if (createMemory() == 0) {
        fail(SIGNAL_MEM_SETUP_FAIL);
    }

    printsize();
    // if(!env_init()) fail(ENV_INIT_FAIL);
    GCinitializeEnv();

    while(1) {
        printf("%d > ", heapAvailableSize());
        // flush for when using pipes 
        fflush(stdout);
        // Read 1 vaild s-expr
        Box ret = Read();
        // TODO: remove temporary leaky check: at top level, pointer registry should be empty
        if (pointerRegistryLeaking()) fail(SIGNAL_POINTER_REGISTRY_LEAKING);
        ret = GCEval(ret);
        // TODO: remove temporary leaky check
        // TODO: remove temporary leaky check: at top level, pointer registry should be empty
        if (pointerRegistryLeaking()) fail(SIGNAL_POINTER_REGISTRY_LEAKING);
        Print(ret);
    }
}
