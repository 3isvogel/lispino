#pragma once

#define STRINGIZE(x) STRINGIZE2(x)
#define STRINGIZE2(x) #x
#define _REF_ __FILE__ ":" STRINGIZE(__LINE__) " "

#define pass return *this
