#pragma once

// apparently doing so is necessary to correctly connect strings
#define STR2(x) #x
#define STR(x) STR2(x)
#define CON2(a, b) (a##b)
#define CON(a, b) CON(a, b)

#define _REF_ __FILE__ ":" STR(__LINE__) " "

#define pass return *this
