#include <stdio.h>
#include "stack.h"
#include "reader.h"
#include "eval.h"
#include "printer.h"
#include "prims.h"
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

#define K 1024
void printsize() {
    int a,b,c,d,e, t;
    a = (TOKENBUF_MAX_LEN+1) * sizeof(char);
    b = (STACK_MAX_LEN) * sizeof(int);
    c = (HEAP_MAX_LEN) * sizeof(int);
    t = a + b + c * 2;
    logInfo("Memory       \tsize\tsize");
    logInfo("Token buffer:\t%d\t(%dK)", a, a/K);
    logInfo("Symbols stack:\t%d\t(%dK)", b, b/K); 
    logInfo("Heaps:      \t%d\t(%dK) x 2", c, c/K); 
    logInfo("Total:      \t%d\t(%dK)", t, t/K); 
}

#define DEFAULT_LOG_LEVEL LOG_LEVEL_ALLOC

int main(int argc, char** argv) {
    logSetLevel(argc > 1 ? atoi(argv[1]) : DEFAULT_LOG_LEVEL);
    if (!init_memory()) fail(MEM_SETUP_FAIL);
    if(!env_init()) fail(ENV_INIT_FAIL);

    logDebug("HEAP%12s [TYPE] | %12s [TYPE]", "car_value", "cdr_value");
    while(1) {
        printf("%d > ", heap_avail());
        // flush for when using pipes 
        fflush(stdout);
        Box ret = Read();
        GC(&ret, 1);
        ret = Eval(ret);
        logDebug("result: %12x [%s]", get_val(ret), type_name[get_tag(ret)]);
        Print(ret);
    }
}
