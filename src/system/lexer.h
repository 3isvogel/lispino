/**
 * @file
 * @brief Lexer interface
 *
 * It's probably better to keep lexer and parser in two different files
 */

// Used by the lexer to communicate types of tokens to the parser
#define TOKEN_TYPE_LIST                 \
X(EOF)      /* Assign a type to EOF */  \
X(LPAR)                                 \
X(RPAR)                                 \
X(STRING)                               \
X(SYMBOL)                               \
X(INT)                                  \
X(LABEL)                                \
X(DOT)                                  \
X(NIL)                                  \
X(QUOTE)

#define X(x) TTYPE_##x,
typedef enum {
    TOKEN_TYPE_LIST \
    TOKEN_TYPE_SIZE
} TokenType;
#undef X

typedef long long int TokenValue;

typedef struct {
    char*           text;
    unsigned int    len;
    TokenType       type;
    TokenValue      value;
    unsigned int    bufferSize;
} Token;

extern Token token;

/**
 * @brief Allocate memory for parser
 *
 * @param bufferSize 
 * @return NULL on fail, non NULL on success
 */
char* createParser(unsigned int bufferSize);

/**
 * @brief Destroy parser memory
 */
void destroyParser();

/**
 * @brief Scan until the next available character
 */
void next();
