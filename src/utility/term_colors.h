#ifndef SRC_UTILS_TERM_COLORS_H
#define SRC_UTILS_TERM_COLORS_H

#include "macros.h"

// modifier codes
#define TERM_CODE_DEFAULT       STR(0)
#define TERM_CODE_BOLD          STR(1)
#define TERM_CODE_LIGHT         STR(2)
#define TERM_CODE_ITALIC        STR(3)
#define TERM_CODE_UNDERLINED    STR(4)
#define TERM_CODE_BLINKING      STR(5)
#define TERM_CODE_REVERSE       STR(7)
#define TERM_CODE_HIDDEN        STR(8)
#define TERM_CODE_STRIKED       STR(9)

// color codes
#define TERM_COLOR_BLACK        0
#define TERM_COLOR_RED          1
#define TERM_COLOR_GREEN        2
#define TERM_COLOR_YELLOW       3
#define TERM_COLOR_BLUE         4
#define TERM_COLOR_PURPLE       5
#define TERM_COLOR_CYAN         6
#define TERM_COLOR_GRAY         7

#define TERM_CODE_FG(color)                     STR(3) STR(color)
#define TERM_CODE_BG(color)                     STR(4) STR(color)

#define TERM_CODE_SET(code)                     "\x1b[" code "m"
#define TERM_CODE_SET2(code1, code2)            "\x1b[" code1 ";" code2 "m"
#define TERM_CODE_SET3(code1, code2, code3)     "\x1b[" code1 ";" code2 ";" code3 "m"
#define TERM_CODE_RESET                         TERM_CODE_SET(TERM_CODE_DEFAULT)

#endif // SRC_UTILS_TERM_COLORS_H
