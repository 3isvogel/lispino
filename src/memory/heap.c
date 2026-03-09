#include "heap.h"
#include <memory/mem.h>
#include <memory/stack.h>
#include <utility/signals.h>
#include <utility/log.h>

#include "memory/private.h"
#include "string.h"
#include "utility/box.h"

#define HEAP_TRESHOLD_GC ((unsigned int)((HEAP_MAX_LEN * sizeof(Box)) - 500))

typedef struct {
    Box* active;
    Box* inactive;
    Box* base;
    unsigned int size;
    // Used in check against size
    unsigned int head;
    // Used to return integer index
    unsigned int bytes;
    unsigned int requested;
} Heap;
Heap heap = (Heap) {
    .base = NULL,
    .active = NULL,
    .inactive = NULL,
    .size = 0,
    .head = 0,
    .requested = 0,
};

typedef struct {
    BoxRef* data;
    unsigned int size;
    unsigned int head;
} Registry;
Registry registry = (Registry) {
    .data = NULL,
    .size = 0,
    .head = 0,
};

void validateRef(BoxRef ref) {
    if (ref < heap.base || ref >= (heap.base + (heap.size*2))) {
        logInfo("Bad reference: %p <= %p <= %p", heap.base, ref, heap.base + (heap.size * 2));
        fail(SIGNAL_BAD_REFERENCE);
    }
}

unsigned int heapAvailableSize() {
    return (heap.size - heap.head) * sizeof(Box);
}

/**
 * @brief Returns a raw pointer to the referenced box
 *
 * @param boxRef 
 * @return 
 */
static inline Box* rawPtr(BoxRef boxRef) {
    // Made generic so implementation can be changed
    validateRef(boxRef);
    return boxRef;
}

Box follow(BoxRef boxRef) {
    validateRef(boxRef);
    return *boxRef;
}

/**
 * @brief Check if the argument references a moved box
 *
 * @param box 
 * @return A reference to the new object if moved, null otherwise
 */
BoxRef checkMoved(BoxRef box);

void destroyHeap() {
    free(heap.base);
    heap.base = NULL;
    heap.active = NULL;
    heap.inactive = NULL;
    heap.head = 0;
    heap.requested = 0;
}

Box* createHeap(unsigned int size) {
    // Check that no heap already exists, if it does delete it
    destroyHeap();
    // Allocate a contiguous block of Box, it will be split into two, referenced
    // by data[0] and data[1], 
    // during GC will refer to these as
    // data[active] and data[inactive]
    //
    // Use first half as the active
    heap.base = (Box*) halloc(size * 2 * sizeof(Box));
    heap.active = heap.base;
    // Use second half as the inactive
    heap.inactive = heap.base + size;
    heap.size = size;
    return heap.base;
}

// Forward declaration
void gc();

// Forward declaration
BoxRef internalMemRequest(unsigned int size) {
    BoxRef reserved = &heap.active[heap.head];
    // Move head accordingly, aligned to Box size;
    unsigned int offset = (size-1)/sizeof(Box) + 1;
    
    heap.head += offset;
    heap.requested += offset * sizeof(Box);

    return reserved;
}

BoxRef newMem(unsigned int size) {
    // TODO: GC will be called here
    if (heap.requested > HEAP_TRESHOLD_GC){
        gc();
    }
    logAlloc("Requesting: %dB", size);
    // If you are requesting no size then NULL is a suitable address
    if (!size) return NULL;
    if (heap.head + size > heap.size)
        fail(SIGNAL_HEAP_FULL);

    // Note: when requesting a block of memory provide at least enough memory
    // to fit a Box
    BoxRef address = internalMemRequest(size);
    logAlloc("Providing:  %dB @ 0x%p", ((size-1)/sizeof(Box) + 1) * sizeof(Box), address);
    return address;
}

Cons* newCons() {
    // Make space for a cons
    Cons* cons = (Cons*)newMem(sizeof(Cons));
    cons->car = boxNil();
    cons->cdr = boxNil();
    return cons;
}

BoxRef newRaw(unsigned int len) {
    // Make space fora a raw string
    BoxRef boxRef = newMem(sizeof(Box) + len + 1);
    setTag(boxRef, TAG_RAW);
    setValue(boxRef, len + 1);
    return boxRef;
}

Box setRaw(BoxRef boxRef, char *string) {
    // Check that enough space was available (ignore padding, ensure string will
    // fit in declared size)
    unsigned int len = strlen(string);
    if (len >= getValue(boxRef)) {
        return boxSignal(SIGNAL_FAIL_RAWMEMORY_CHECK);
    }
    memcpy(boxRef + 1, string, len+1);
    setValue(boxRef, len+1);
    return setBox((Value)boxRef, TAG_NIL);
}

// TODO: deref check
char* getRaw(Box box) {
    BoxRef ref = (BoxRef)getValue(&box);
    return (char*)(ref+1);
}

void moveBox(Box* box);

void gc() {
    logDebug("Called GC: used %d", heap.requested);

    // Sadly C won't let me do bitwise unless I am veeeeeery verbose
    // Use swap active and inactive pointer, consider an empty heap and start
    // filling it
    Box* t = heap.active;
    heap.active = heap.inactive;
    heap.inactive = t;
    heap.head = 0;

    Frame frame = frameCurrent();

    // Scan all frames and move all accessible data to new active buffer
    do {
        for(Cons* ptr = frame.start; ptr < frame.end; ptr++) {
            // If something is not a ref it will not be moved
            moveBox(&(ptr->car));
            moveBox(&(ptr->cdr));
        }
    } while(frameOuter(&frame));

    // Move all heap accessible from registered pointers
    for(unsigned int i = 0; i < registry.head; i++) {

        // boxRef is referring to the actual memory in the C program stack
        BoxRef boxRef = registry.data[i];
        // Ensure that the box is moved (will only move if does not contain an
        // immediate, otherwise return without doing anything, if the value is
        // moved already returns without doing anything)
        // 
        // This function treats each Box as an entry on the symbol stack, moving
        // only elements that need to be moved, ignoring the others and updating
        // references when needed
        moveBox(registry.data[i]);
    }

    // After the clean reset the amount of bytes used
    heap.requested = heap.head * sizeof(Box);
    logInfo("After GC: used %d", heap.requested);

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
 *  X.AAA, indicating that this is a moved value (tag X) wich is now in memory
 *  address pointed by AAA, however, we call the function from Z.000, and this
 *  is only done when value is not an immediate
 */
BoxRef checkMoved(BoxRef box) {
    // It makes sense to call this only if the current box is a non-immediate
    // (string, symbols, cons, closure, etc.)
    //
    // Follow the box as a reference
    Box oldBox = follow((BoxRef)getValue(box));
    // If referencing to a moved object
    if(getTag(&oldBox) == TAG_MOVED) {
        // Return its new location
        return (BoxRef) getValue(&oldBox);
    }
    // Otherwise it was not moved, return an invalid reference
    return NULL;
}

BoxRef moveCons(BoxRef oldCons) {
    // copy from the original cons to a newly-requested memory address
    BoxRef newCons = internalMemRequest(sizeof(Cons));
    memcpy(newCons, oldCons, sizeof(Cons));

    // Flag the old cons as moved
    *rawPtr(oldCons) = setBox((Value)newCons, TAG_MOVED);
 
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
BoxRef moveRaw(BoxRef oldString) {

    // Calculate total string length
    Box oldRaw = follow(oldString);
    unsigned int len = sizeof(Box) + getValue(&oldRaw) + 1;

    // Request a new memory area that is big enough to fit a string type
    // (Box metadata + raw string) + '\0'
    BoxRef newString = internalMemRequest(len);
    memcpy(newString, oldString, len);

    // Flag the old string as moved
    *rawPtr(oldString) = setBox((Value)newString, TAG_MOVED);
    return newString;
}

void moveBox(BoxRef box) {
    // Need definition at top since cannot define inside a switch block
    BoxRef newRef;
    switch(getTag(box)) {
        case TAG_CONS:
        case TAG_CLOSURE:
            if((newRef = checkMoved(box))){
                validateRef(newRef);
                // The addres this box is pointing to has been moved to another
                // location already, insead of copying it just update the old
                // reference
                setValue(box, (Value)newRef);
                // The tag doesn't change on GC
                //setTag(box, getTag(moved));
                // TODO: check if returning is necessary
                return;
            }
            // If the value was not moved then the addres pointed by this box is
            // a cons which is still in the old buffer: move it to the new one
            newRef = moveCons((BoxRef) getValue(box));
            // Update reference
            setValue(box, (Value)newRef);
            // Recursively move car and cdr
            moveBox(newRef);
            // TCO BABY!!!
            // (I hope so)
            return moveBox(&newRef[1]);
        case TAG_STRING:
        case TAG_SYMBOL:
        case TAG_LABEL:
            // As for CONS: check if the value was moved, in which case just
            // link the value
            if((newRef = checkMoved(box))) {
                // Update reference
                setValue(box, (Value)newRef);
                return;
            }
            // Otherwise move the string to the new position
            newRef = moveRaw((BoxRef) getValue(box));
            setValue(box, (Value)newRef);
        default:
            // For all atomic values there is no need to copy them, they reside
            // in the cons itself and will be copied
            return;
    }
}

void destroyPointerRegistry() {
    if (registry.data != NULL)
        free(registry.data);
    registry.data = NULL;
    registry.head = 0;
}

unsigned int createPointerRegistry(unsigned int size) {
    destroyPointerRegistry();

    registry.data = (BoxRef*) halloc(size * sizeof(BoxRef));
    registry.size = size;
    registry.head = 0;
    return 1;
}

void pointerRegistryPush(BoxRef boxRef) {
    if (registry.head == registry.size)
        fail(SIGNAL_POINTER_REGISTRY_FULL);
    registry.data[registry.head ++ ] = boxRef;
}

void pointerRegistryPop() {
    if (registry.head > 0) registry.head --;
}

void pointerRegistryReset() {
    registry.head = 0;
}
