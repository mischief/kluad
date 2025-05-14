#include <lua.h>

typedef struct genlib {
	lua_CFunction f;
	const char *name;
} genlib;

extern genlib genlib_list[];

