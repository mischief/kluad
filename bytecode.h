#include <stddef.h>

typedef struct bytecode {
	const char *code;
	size_t length;
	char *name;
} bytecode;

extern bytecode bytecode_list[];

