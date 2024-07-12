#include <stdio.h>
#include "stack.h"
#include "reader.h"
#include "printer.h"
#include "heap.h"
#include "log.h"

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
    logInfo("Heaps:      \t%d\t(%dK)", c, c/K); 
    logInfo("Total:      \t%d\t(%dK)", t, t/K); 
}

int main() {
    type_init();
    logSetLevel(LOG_LEVEL_DEBUG);
    printsize();

    logDebug("Heap cells");
    logDebug("    %12s [TYPE] | %12s [TYPE]", "car_value", "cdr_value");
    while(1) {
        printf("%d > ", heap_avail());
        Box ret = Read();
        logDebug("%12x [%d]", get_val(ret), get_tag(ret));
        Print(ret);
    }
}
