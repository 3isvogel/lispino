#include "heap.h"
#include <memory/mem.h>
#include <memory/stack.h>
#include <utility/signals.h>
#include <utility/log.h>
#include <utility/hash.h>

#include "memory/private.h"
#include "string.h"
#include "utility/box.h"

// TODO: decide a good treshold
#define HEAP_TRESHOLD_GC ((unsigned int)((HEAP_MAX_LEN * sizeof(Box))*3/4))

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

// Raw strings collision map data structure
typedef struct {
    BoxRef *rawRefs;
    unsigned int *keys;
    unsigned int size;
    unsigned int entries;
    unsigned int mask;
    unsigned int probe;
} RawStringMap;
RawStringMap rawMap = (RawStringMap) {
    .rawRefs = NULL,
    .keys = NULL,
    .size = 0,
    .mask = 0,
    .probe = 0,
    .entries = 0,
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
 * @brief Deallocate rawstring map memory if it's allocated
 */
void destroyRawStringMap();
BoxRef* createRawStringMap(unsigned int minSize);

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
    createRawStringMap(STACK_MAX_LEN);
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
    return setBox((Value)boxRef, TAG_NIL);
}

// TODO: deref check
char* getRaw(Box box) {
    BoxRef ref = (BoxRef)getValue(&box);
    return (char*)(ref+1);
}

void moveBox(Box* box);
void cleanRawStringMap();

/**
 * @brief Check if rawRef exists inside rawStringMap
 *
 * @param rawRef 
 * @return 
 */
BoxRef getRawStringMap(BoxRef rawRef);

/**
 * @brief Insert rawRef into rawStringMap
 *
 * @param rawRef 
 */
void insertRawStringMap(BoxRef rawRef);

void gc() {
    logDebug("Called GC: used %d", heap.requested);

    // Sadly C won't let me do bitwise unless I am veeeeeery verbose
    // Use swap active and inactive pointer, consider an empty heap and start
    // filling it
    Box* t = heap.active;
    heap.active = heap.inactive;
    heap.inactive = t;
    heap.head = 0;

    // Clean raw string map for raw string deduplication
    cleanRawStringMap();

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
            // Before copying the raw to the new heap: check that a raw with the
            // same value doesn't exist already Otherwise simply point the
            // string to the new position

            // If the string already exists simply update the reference
            if ((newRef = getRawStringMap((BoxRef)getValue(box)))) {
                setValue(box, (Value)newRef);
                return;
            }
            // If the string does not exist in the map copy it into the new heap
            newRef = moveRaw((BoxRef) getValue(box));
            setValue(box, (Value)newRef);
            // And add it to the map
            insertRawStringMap(newRef);
            return;
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
    if (registry.head == 0) fail(SIGNAL_POINTER_REGISTRY_EMPTY);
        registry.head --;
}

void pointerRegistryReset() {
    registry.head = 0;
}

// Maybe using reset at every REPL cycle is too much, check if registry is
// leaking pointers
unsigned int pointerRegistryLeaking() {
    return registry.head;
}


unsigned int getRawStringMapSize() {
    return rawMap.size;
}

void destroyRawStringMap() {
    if (rawMap.rawRefs) free(rawMap.rawRefs);
    rawMap.rawRefs = NULL;
    rawMap.keys = NULL;
    rawMap.size = 0;
    rawMap.mask = 0;
    rawMap.probe = 0;
}

BoxRef* createRawStringMap(unsigned int minSize) {

    destroyRawStringMap();

    unsigned int halfWordSize = (int)(sizeof(BoxRef) * 8 / 2);
    // I don't want to round up, I want to go to the next power of 2
    unsigned int size = minSize;// << 1;

    // Copied from
    // https://graphics.stanford.edu/%7Eseander/bithacks.html#RoundUpPowerOf2    
    // size --;
    for (unsigned int i = 1; i < halfWordSize; i *= 2) {
        size |= size >> i;
    }
    size ++;

    if (size == 0)
        fail(SIGNAL_MEM_SETUP_FAIL);
    rawMap.probe = primeProbe((unsigned int)(size*3/4));
    if (rawMap.probe == 0)
        fail(SIGNAL_MEM_SETUP_FAIL);
    
    // Allocate contiguous memory, but use 2 separated arrays so I only need to
    // wipe one of them, rawRef points to the beginning of memory up to
    // rawRef + (size * sizeof(BoxRef))
    // while keys points to the end of rawRefs up to
    // keys + (size * sizeof(unsigned int));
    rawMap.rawRefs = (BoxRef*) halloc(size * (sizeof(BoxRef) + sizeof(unsigned int)));
    rawMap.keys = (unsigned int*) (rawMap.rawRefs + size);
    logInfo("RawStringMap size: %d probe offset: %d", size, rawMap.probe);
    rawMap.size = size;
    rawMap.mask = size - 1;
    return (BoxRef*)rawMap.rawRefs;
}

void cleanRawStringMap() {
    rawMap.entries = 0;
    memset(rawMap.rawRefs, 0, rawMap.size * sizeof(BoxRef));
}

/**
 * @brief Compare two rawRef
 *
 * Returns 0 if they have different content, non 0 otherwise
 *
 * @param rawRefA 
 * @param rawRefB 
 * @return 
 */
unsigned int rawEq(BoxRef rawRefA, BoxRef rawRefB) {
    // Same pointer: are the same
    if (rawRefA == rawRefB) return 1;
    // Different length: cannot be the same
    if (getValue(rawRefA) != getValue(rawRefB)) return 0;
    // String compare
    return !strcmp((char*)(rawRefA+1), (char*)(rawRefB+1));
}

BoxRef getRawStringMap(BoxRef rawRef) {
    char *string = (char*)(rawRef+1);
    unsigned int len = getValue(rawRef);
    unsigned int key = hash(string, len);
    // Scan N (number of entries presents) bucket, no need to scan more
    // 
    // Use key as key value, probe as index access
    for (unsigned int i = 0, probe = key & rawMap.mask
            ; i < rawMap.entries
            ; i++, probe = (probe + rawMap.probe) & rawMap.mask) {
        // If bucket has empty ref -> not exists in hashmap
        if (rawMap.rawRefs[probe] == NULL) return NULL;
        // If both key match and the rawString is the same (either by addres
        // or value) returns the found
        // The idea is: lazy evaluation will skip most rawEq
        // (assuming key != (key & mask), which should be the case)
        if (key == rawMap.keys[probe] && rawEq(rawRef, rawMap.rawRefs[probe]))
            return rawMap.rawRefs[probe];
    }
    return NULL;
}

/**
 * @brief Insert rawRef into rawstringmap
 *
 * @param rawRef 
 */
void insertRawStringMap(BoxRef rawRef) {
    char *string = (char*)(rawRef+1);
    unsigned int len = getValue(rawRef);
    if (rawMap.entries == rawMap.size) fail(SIGNAL_RAW_MAP_FULL);
    unsigned int key = hash(string, len);
    for (unsigned int i = 0, probe = key & rawMap.mask
            // Necessary + 1 as otherwise I will never do this
            ; i < rawMap.entries + 1
            ; i++, probe = (probe + rawMap.probe) & rawMap.mask) {
        // Empty bucket, populate it and return
        if (rawMap.rawRefs[probe] == NULL) {
            rawMap.rawRefs[probe] = rawRef;
            rawMap.keys[probe] = key;
            rawMap.entries ++;
            return;
        }
    }
    // NOTE: I know this is not the correct implementation: multiple inserts
    //       should discard one of the two values (in this case preserve the one
    //       already present and discard the latter, but I will only call this
    //       insert after the get, so I am sure the value is not already in the
    //       map, to be corret I should add a check that the value already
    //       exists and discard it
}
