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
        bindStackSize,
        rawMapSize,
        totalSize;
    tokenBufferSize = (TOKEN_BUFFER_MAX_LEN+1) * sizeof(char);
    stackSize = (STACK_MAX_LEN) * sizeof(Cons);
    heapSize = (HEAP_MAX_LEN) * sizeof(Box) * 2;
    rawMapSize = getRawStringMapSize() * sizeof(BoxRef);
    bindStackSize = (BIND_STACK_MAX_LEN) * sizeof(Box);
    pointerRegistrySize = (POINTER_REGISTRY_MAX_LEN) * sizeof(Box*);

    // Using two heaps: copy GC
    totalSize = tokenBufferSize + stackSize + heapSize + pointerRegistrySize + rawMapSize + pointerRegistrySize;

    logInfo("%18s %10s %10s", "Memory", "size (B)", "size (KB)");
    logInfo("%18s %10u %10u", "Token buffer:",      tokenBufferSize, tokenBufferSize/K);
    logInfo("%18s %10u %10u", "Symbols stack:",     stackSize, stackSize/K);
    logInfo("%18s %10u %10u", "Heaps (x2):",        heapSize, (heapSize/K));
    logInfo("%18s %10u %10u", "Pointer registry:",  pointerRegistrySize, pointerRegistrySize/K);
    logInfo("%18s %10u %10u", "Bind Stack:",        bindStackSize, bindStackSize/K);
    logInfo("%18s %10u %10u", "Raw String Map:",    rawMapSize, rawMapSize/K);
    logInfo("");
    logInfo("%18s %10u %10u", "Total:",             totalSize, totalSize/K);
    logInfo("");
    logInfo("%18s %10u", "Box:", sizeof(Box));
    logInfo("%18s %10u", "Cons cell:", sizeof(Cons));
}

int main(int argc, char** argv) {

    logSetLevel(LOG_LEVEL_DEBUG);
    logSetLevel(LOG_LEVEL_INFO);

    if (createMemory() == 0) {
        fail(SIGNAL_MEM_SETUP_FAIL);
    }

    printsize();
    GCinitializeEnv();

    while(1) {
        fprintf(stderr, "%d > ", heapAvailableSize());
        // flush for when using pipes 
        fflush(stdout);
        // Read 1 vaild s-expr
        Box ret = Read();
        // TODO: remove temporary leaky check: at top level, pointer registry should be empty
        assert(pointerRegistryLeaking() == 0);
        ret = GCEval(ret);
        // TODO: remove temporary leaky check: at top level, pointer registry should be empty
        assert(pointerRegistryLeaking() == 0);
        Print(ret, stderr);
    }
}
