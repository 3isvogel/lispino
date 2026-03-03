#include <stdio.h>
#include "stack.h"
#include "reader.h"
#include "eval.h"
#include "printer.h"
#include "errors.h"
#include "heap.h"
#include "log.h"
#include "mem.h"

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
        d,
        e,
        totalSize;
    tokenBufferSize = (TOKENBUF_MAX_LEN+1) * sizeof(char);
    stackSize = (STACK_MAX_LEN) * sizeof(int);
    heapSize = (HEAP_MAX_LEN) * sizeof(int);
    // Using two heaps: copy GC
    totalSize = tokenBufferSize + stackSize + heapSize * 2;

    logInfo("Memory       \tsize\tsize");
    logInfo("Token buffer:\t%d\t(%dK)", tokenBufferSize, tokenBufferSize/K);
    logInfo("Symbols stack:\t%d\t(%dK)", stackSize, stackSize/K); 
    logInfo("Heaps:      \t%d\t(%dK) x 2", heapSize, heapSize/K); 
    logInfo("Total:      \t%d\t(%dK)", totalSize, totalSize/K);
}

#define DEFAULT_LOG_LEVEL LOG_LEVEL_ALLOC

int main(int argc, char** argv) {

    if (argc > 1) {
        logSetLevel(atoi(argv[1]));
    } else {
        logSetLevel(DEFAULT_LOG_LEVEL);
    }

    if (init_memory() == 0) {
        fail(MEM_SETUP_FAIL);
    }
    if(!env_init()) fail(ENV_INIT_FAIL);

    logDebug("HEAP%12s [TYPE] | %12s [TYPE]", "car_value", "cdr_value");

    while(1) {
        printf("%d > ", heap_avail());
        // flush for when using pipes 
        fflush(stdout);
        // Read 1 vaild s-expr
        Box ret = Read();
        ret = Eval(ret);
        logDebug("result: %12x [%s]", get_val(ret), type_name[get_tag(ret)]);
        Print(ret);
    }
}
