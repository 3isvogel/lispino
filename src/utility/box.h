/**
 * @file
 * @brief Hide implementation of a box, so changing implementation doesn't
 * disrupt the code
 */

#pragma once

#define TAG_LIST \
X(NIL) \
X(INT) \
X(F64) \
X(SYMBOL) \
X(CONS) \
X(PRIMITIVE) \
X(CLOSURE) \
X(LABEL) \
X(MOVED) \
X(RAW) \
X(STRING) \
X(SIGNAL)

#define X(x) TAG_##x,
typedef enum
// TODO: Consider if it's oki
__attribute__((__packed__))
{
    TAG_LIST
    TAG_SIZE
} Tag;
#undef X

// Using NaN boxing it's possible to fit almost anything into a double, I will
// use a more understandable representation and then try to use the more
// optimized one
// typedef double Box;

typedef long long int Value;

typedef struct
// TODO: Consider if it's oki
__attribute__((__packed__))
{
    // Doesn't really matter which type this has, as long as it fits 64 bits
    Value value;
    // Doesn't really matter which type this has, used as a mask
    Tag tag;
} Box;

// Abstract pointers when working outside of heap, always perform a boundary
// check to make sure that the reference lays withing machine's heap
typedef Box *BoxRef;

// Define type Function as any function pointer that accepts a BoxRef as an
// argument and returns a box
typedef void (*Function)(BoxRef, Box);

// A cons contains two boxes: a car and a cdr
typedef struct {
    Box car,
        cdr;
} Cons;

/**
 * @brief returns the value of a box
 *
 * @param box 
 * @return 
 */
Value getValue(BoxRef box);

/**
 * @brief returns the tag of a box
 *
 * @param box 
 * @return 
 */
Tag getTag(BoxRef box);

/**
 * @brief sets the value of a box
 *
 * @param box 
 * @param value 
 */
void setValue(BoxRef box, Value value);

/**
 * @brief sets the tag of a box
 *
 * @param box 
 * @param tag 
 */
void setTag(BoxRef box, Tag tag);

/**
 * @brief create a box with specified value and tag
 *
 * @param value 
 * @param tag 
 * @return 
 */
Box setBox(Value value, Tag tag);

/**
 * @brief Returns a nil box
 *
 * Useful to initialize boxes and preventing GC to go curazy
 *
 * @return 
 */
static inline Box boxNil() {
    return setBox(0, TAG_NIL);
}

/**
 * @brief returns the printable verion of a tag
 *
 * @param tag 
 * @return 
 */
char* strTag(Tag tag);


/**
 * @brief Returns the car of a box referencing a cons
 *
 * @param boxRef 
 * @return 
 */
Box getCar(BoxRef boxRef);

/**
 * @brief Returns the cdr of a box referencing a cons
 *
 * @param boxRef 
 * @return 
 */
Box getCdr(BoxRef boxRef);

/**
 * @brief Sets the car of a box referencing a cons
 *
 * @param boxRef 
 * @param value 
 */
void setCar(BoxRef boxRef, Box value);

/**
 * @brief Sets the cdr of a box referencinga cons
 *
 * @param boxRef 
 * @param value 
 */
void setCdr(BoxRef boxRef, Box value);
