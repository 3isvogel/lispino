/**
 * @file
 * @brief Lexer implementation
 */

#include "lexer.h"

#include <utility/box.h>
#include <utility/log.h>
#include <utility/signals.h>

#include <memory/mem.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Token token = (Token) {
    .text = NULL,
};

void destroyParser() {
    free(token.text);
}

/**
 * @brief Allocate memory for the parser
 *
 * @param maximumTokenSize 
 * @return pointer to allocated buffer, used by memory management to free it
 * later
 */
char* createParser(unsigned int bufferSize) {
    if (token.text != NULL) destroyParser();

    char* buffer = (char*)halloc(bufferSize + 1);

    token = (Token) {
        .text = buffer,
        .len = 0,
        .bufferSize = bufferSize,
        .type = TTYPE_NIL,
        .value = 0,
    };
    return buffer;
}

/**
 * @brief Append character to the token safely
 *        TODO: (needs refining)
 *
 * @param character 
 */
void appendChar(char character) {
    if (token.len == token.bufferSize) {
        logError("; Token too long: %.*s", token.len, token.text);
        fail(SIGNAL_TOKEN_TOO_LONG);
    }
    token.text[token.len ++ ] = character;
}

static inline void finalizeToken() {
    appendChar('\0');
    token.len --;
}

void clearToken() {
    token.len = 0;
}

/**
 * @brief Consume input until a matching token is found
 *
 * @return 
 */
int cc = '\0';
void next() {

    clearToken();

    do {
        
        // End of file
        if (cc == EOF) {
            token.type = TTYPE_EOF;
            appendChar('\0');
            goto lexerReturn;
        // ( Left parenthesis
        } else if (cc == '(') {
            token.type = TTYPE_LPAR;
            appendChar(cc);
            goto lexerConsumeAndReturn;
        // ) Right parenthesis
        } else if (cc == ')') {
            token.type = TTYPE_RPAR;
            appendChar(cc);
            goto lexerConsumeAndReturn;
        // . Dot
        } else if (cc == '.') {
            token.type = TTYPE_DOT;
            appendChar(cc);
            goto lexerConsumeAndReturn;
        // ' Single quote
        } else if (cc == '\'') {
            token.type = TTYPE_QUOTE;
            appendChar(cc);
            goto lexerConsumeAndReturn;
        // "xxxx" String
        } else if (cc ==  '"') {
            cc = getchar();
            while (cc != '"'){
                if (cc == '\\') {
                    appendChar('\\');
                    cc = getchar();
                    //static const char *escapeChars = "abtnvfr";
                    //const char *escapedLetter = strchr(escapeChars, cc);
                    //appendChar(escapedLetter ? escapedLetter - escapeChars : cc);
                }
                appendChar(cc);
                cc = getchar();
            }
            token.type = TTYPE_STRING;
            goto lexerConsumeAndReturn;
            // Keep adding character until a non-escaped " is found
        // ; Comments
        } else if (cc ==  ';') {
            while((cc = getchar()) != '\n') {};
            // Also consume '\n'
        // Numbers
        } else if ((cc >= '0' && cc <= '9') || cc == '-' || cc == '+') {
            do {
                appendChar(cc);
                cc = getchar();
            } while(cc >= '0' && cc <= '9');
            // It's a number
            if (token.len > 1 || (token.text[0] >= '0' && token.text[0] <= '9')) {
                token.type = TTYPE_INT;
                finalizeToken();
                token.value = strtoul(token.text, NULL, 10);
                goto lexerReturn;
            // Whoops, it was a symbol
            } else {
                goto actuallyASymbol;
            }
        // Symbols and labels
        } else if (cc >= '*' && cc <= '~') {
            do {
                appendChar(cc);
                cc = getchar();
actuallyASymbol:
                // Hey did you know that "Label at end of compound statement is a C23 extension"?
                asm("nop");
            } while(cc >= '*' && cc <= '~' && cc != ';');
            if (token.text[0] == ':')
                token.type = TTYPE_LABEL;
            else
                token.type = TTYPE_SYMBOL;
            goto lexerReturn;
        } else if (cc >= '!' && cc <= '\'') {
            todo("Implement special characters");
        }
        
        // If it doesn't match anything just consume it
        cc = getchar();

    } while(1);

// Cerain token (like symbols) keep reading until an invalid character is found
// so they will jump in lexerReturn (cc is already outside of token)
// While other (like parenthesis and strings) are single-character or have
// well-defined begin-end, so they don't need to consume an additional character
// to ensure both work the same way, the latter kind will jump to
// lexerConsumeAndReturn
lexerConsumeAndReturn:
    cc = getchar();
lexerReturn:
    finalizeToken();
    // Putch increases size (makes '\0' part of the length)
    return;
}
