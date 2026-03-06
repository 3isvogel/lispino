#include "heap.h"
#include "memory/mem.h"
#include "string.h"
#include <utility/signals.h>
#include <utility/log.h>

#include <stdlib.h>

#define HEAP_TRESHOLD_GC 0x4000

typedef struct {
    Box* active;
    Box* inactive;
    unsigned int size;
    unsigned int head;
    unsigned int bytes;
    unsigned int requested;
} Heap;
Heap heap = (Heap) {
    .active = NULL,
    .inactive = NULL,
    .size = 0,
    .head = 0,
    .requested = 0,
};

typedef struct {
    Box*** data;
    unsigned int size;
    unsigned int head;
} Registry;
Registry registry = (Registry) {
    .data = NULL,
    .size = 0,
    .head = 0,
};

/**
 * @brief Deallocate the heap if it exists
 */
void destroyHeap() {
    // Should NEVER occur that one of the two is a value while the other is NULL
    if(heap.active < heap.inactive) {
        free(heap.active);
    } else {
        free(heap.inactive);
    }
    // If they are equals no memory was allocated (These two memory areas cannot
    // overlap)
    heap.active = NULL;
    heap.inactive = NULL;
    heap.head = 0;
    heap.requested = 0;
}

/**
 * @brief Allocate a new heap
 *
 * @param size 
 * @return A pointer to cons, indicating if the operation was successful or not
 */
Box* createHeap(unsigned int size) {
    // Check that no heap already exists, if it does delete it
    destroyHeap();
    // Allocate a contiguous block of Box, it will be split into two, referenced
    // by data[0] and data[1], 
    // during GC will refer to these as
    // data[active] and data[inactive]
    //
    // Use first half as the active
    heap.active = (Box*) halloc(size * sizeof(Box));
    // Use second half as the inactive
    heap.inactive = heap.active + size;
    return heap.active;
}

// Forward declaration
/**
 * @brief Copy garbage collect
 */
void gc();

// Forward declaration
/**
 * @brief Internal memory requests: skips garbage collector
 *
 * @param size 
 * @return The address of th allocated memory
 */
Box* internalMemRequest(unsigned int size);

/**
 * @brief Calls garbage collector and requests a specified amount of bytes
 *
 * @param size 
 * @return The address of the allocated memory
 */
Box* memRequest(unsigned int size) {
    // TODO: GC will be called here
    if (heap.requested > HEAP_TRESHOLD_GC){
        todo("Call GC");
        gc();
    }
    logAlloc("Requesting: %dB", size);
    // If you are requesting no size then NULL is a suitable address
    if (!size) return NULL;
    if (heap.head + size > heap.size)
        fail(SIGNAL_HEAP_FULL);

    // Note: when requesting a block of memory provide at least enough memory
    // to fit a Box
    logAlloc("Providing:  %dB", ((size-1)/sizeof(Box) + 1) * sizeof(Box));
    return internalMemRequest(size);
}

Box* internalMemRequest(unsigned int size) {
    Box* reserved = &heap.active[heap.head];
    // Move head accordingly, aligned to Box size;
    heap.head += (size-1)/sizeof(Box) + 1;
    
    heap.requested += size;

    return reserved;
}

/**
 * @brief the box to a new location, marking the old one as moved
 *
 * @param box 
 */

void moveBox(Box* box);

void gc() {
    logDebug("Called GC");

    if(!stack_base) find_stack_base();
    Cell stack_top = get_env()->stack;
    // after this i know where is stack base and stack top

    // Sadly C won't let me do bitwise unless I am veeeeeery verbose
    // Use swap active and inactive pointer, consider an empty heap and start
    // filling it

    Box* t = heap.active;
    heap.active = heap.inactive;
    heap.inactive = t;
    heap.head = 0;

    // Move all heap accessible from stack
    for(Cell* x = stackRoot; x < stackTop; x++) {
        // Scan the whole stack,
        if( /* x is an address in the inactive (was active) heap range */ ) {
            // Recursively move all data in the heap that is reachable from this
            // pointer
            // x->name = moveName(root->name);
            // x-> = moveBox(root->name);
        }
    }

    // Move all heap accessiblefrom registered pointers
    for(Cell* x = registeredBase; x < registeredTop; x++) {
        // Scan the whole stack,
        if( /* x is an address in the inactive (was active) heap range */ ) {
            // Recursively move all data in the heap that is reachable from this
            // pointer
            // x->name = moveName(root->name);
            // x-> = moveBox(root->name);
            // TODO: UPDATE POINTERS
        }
    }

}

/*
 *  Z.000
 *  __________   __________
 * |X.AAA     | |Y.BBB     |
 * |          | |          |
 * |          | |          |
 * |__________| |__________|
 *  old          new
 *
 *  The old memory buffer originally holds data Y.BBB (tag.value), once this
 *  content is moved to the new memory buffer, the old value is replaced by
 *  X.AAA, indicating that this is a mooved value (tag X) wich is now in memory
 *  address pointed by AAA, however, we call the function from Z.000, and this
 *  is only done when value is not an immediate
 */

/**
 * @brief If the box is moved return it's new location
 *
 * To prevent duplicatingmemory, every time a box is moved it's original value
 * is set to its new address, with the special tag "TAG_MOVED"
 *
 * @param box
 * @return New box location
 */
Box* checkMoved(Box* box) {
    // It makes sense to call this only if the current box is a non-immediate
    // (string, symbols, cons, closure, etc.)
    //
    // If the box I'm pointing to has been moved
    Box* old = (Box*) getValue(box);
    if(getTag(old) == TAG_MOVED) {
        // Return its new location
        return (Box*) getValue(old);
    }
    // Otherwise it was not moved
    return NULL;
}

/**
 * @brief Copy cons to the new heap buffer, flagging the previous value as moved
 *
 * @param cons 
 * @return Address of the new value
 */
Cons* moveCons(Cons* cons) {
    // memcopy from the original cons to a newly-requested memory address
    Cons* newCons = (Cons*)internalMemRequest(sizeof(Cons));
    *newCons = *cons;
    // Flag the old cons as moved
    *((Box*)cons) = box((Value)newCons, TAG_MOVED);
 
   return newCons;
}

/* 
 * Strings are treated similarly to boxex, with the difference that a string is
 * a sequence of a box X.AAA.ssss... (Box of tag X, value AAA (indicating the
 * length of the string) immediately followed by ssss string.
 *
 * Therefore fom
 * Z.000 pointing to X.AAA.ssss... will point to Y.BBB.ssss...
 */

/**
 * @brief Copy string to the new heap buffer, flagging the previous value as
 * moved
 *
 * @param string 
 * @return 
 */
Box* moveString(Box* string) {
    unsigned long long int len = getValue(string);
    // Request a new memory area that is big enough to fit a string type
    // (Box metadata + raw string) + '\0'
    Box* newString = (Box*) memcpy(internalMemRequest(sizeof(Box) + len + 1),
                                   string, sizeof(Box) + len + 1);
    *string = box((Value)newString, TAG_MOVED);
    return newString;
}

void moveBox(Box* box) {
    // Need definition at top since cannot define inside a switch block
    Cons* newNode;
    Box* newString;
    Box* moved;
    switch(getTag(box)) {
        case TAG_CONS:
        case TAG_CLOSURE:
            if((moved = checkMoved(box))){
                // The addres this box is pointing to has been moved to another
                // location already, insead of copying it just update the old
                // reference
                setValue(box, getValue(moved));
                // The tag doesn't change on GC
                //setTag(box, getTag(moved));
                // TODO: check if returning is necessary
                return;
            }
            // If the value was not moved then the addres pointed by this box is
            // a cons which is still in the old buffer: move it to the new one
            newNode = moveCons((Cons*) getValue(box));
            // Recursively move car and cdr
            moveBox(&(newNode->car));
            setValue(box, (Value)newNode);
            // TCO BABY!!!
            // (I hope so)
            return moveBox(&(newNode->cdr));
        case TAG_STRING:
        case TAG_SYMBOL:
        case TAG_LABEL:
            // As for CONS: check if the value was moved, in which case just
            // link the value
            if((moved = checkMoved(box))) {
                setValue(box, getValue(moved));
                return;
            }
            // Otherwise move the string to the new position
            newString = moveString((Box*) getValue(box));
            setValue(box, (Value) newString);
            return;
        default:
            return;
    }
    // For all atomic values there is no need to copy them, they reside
    // in the cons itself and will be copied
}

/**
 * @brief Destroy pointer registry if it exitss
 */
void destroyPointerRegistry() {
    if (registry.data != NULL)
        free(registry.data);
    registry.data = NULL;
}

/**
 * @brief Allocate a new pointer regitsry
 *
 * @param size 
 */
unsigned int createPointerRegistry(unsigned int size) {
    destroyPointerRegistry();

    registry.data = (Box***) halloc(size * sizeof(Box**));
    return 1;
}

/**
 * @brief Register a box pointer to the registry
 *
 * @param boxPtr
 */
void pointerRegistryPush(Box** boxPtr) {
    if (registry.head == registry.size)
        fail(SIGNAL_POINTER_REGISTRY_FULL);
    registry.data[registry.head ++ ] = boxPtr;
}

/**
 * @brief pop the last pointer in the registry
 */
void pointerRegistryPop();
