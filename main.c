#include <sys/types.h> /* size_t, ssize_t */
#include <signal.h>
#include <stdarg.h> /* va_list */
#include <stddef.h> /* NULL */
#include <stdint.h> /* int64_t */
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <getopt.h>
#include <errno.h>

#include <event.h>

#include <kcgi.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "bytecode.h"
#include "genlib.h"

#define cleanup(x) __attribute__((cleanup(x)))

/* TODO:
 * - json/markdown?
 * - imsg
 */

typedef struct kluad_context
{
	struct kfcgi *fcgi;
	lua_State *L;
	struct event ev;

	int is_dev;
} kluad_context;

int is_dev = 0;

static void
lua_closep(lua_State **L)
{
	if(L)
		lua_close(*L);
}

static int
lua_setup(lua_State **LL, const char *initf)
{
	lua_State *L;
	bytecode *b;
	genlib *g;

	L = luaL_newstate();

	if(!L)
		return -ENOMEM;

	luaL_openlibs(L);

	g = genlib_list;
	while(g->f){
		kutil_info(NULL, NULL, "preloading native module %s", g->name);

		lua_getglobal(L, "package");
		lua_getfield(L, -1, "preload");
		lua_replace(L, -2);
		lua_pushcfunction(L, g->f);
		lua_setfield(L, -2, g->name);
		lua_pop(L, 1);

		g++;
	}

	b = bytecode_list;
	while(b->code){
		kutil_info(NULL, NULL, "preloading lua bytecode %s", b->name);

		if(luaL_loadbuffer(L, b->code, b->length, b->name) != LUA_OK)
			goto erroutb;

		/* set package.preload[name] = chunk */
		lua_getglobal(L, "package");
		lua_getfield(L, -1, "preload");
		lua_replace(L, -2);
		lua_pushvalue(L, -2);
		lua_setfield(L, -2, b->name);
	
		b++;
		continue;

		if(lua_pcall(L, 0, 1, 0) != LUA_OK)
			goto erroutb;

erroutb:
		kutil_warnx(NULL, NULL, "setup: bytecode %s: %s", b->name, lua_tostring(L, -1));
		lua_pop(L, 1);
		lua_close(L);

		return -EILSEQ;
	}

	kutil_info(NULL, NULL, "loading entrypoint %s", initf);

	if(luaL_dofile(L, initf) != LUA_OK){
		kutil_warnx(NULL, NULL, "setup: entrpoint %s: %s", initf, lua_tostring(L, -1));
		lua_pop(L, 1);
		lua_close(L);

		return -EILSEQ;
	}

	/* clean stack */
	lua_settop(L, 0);

	*LL = L;

	return 0;

}

static void
usage()
{
	dprintf(2, "usage: %s [--dev] /path/to/init.lua\n\n", getprogname());
	dprintf(2, "  --dev:     development mode\n");
	exit(EX_USAGE);
}

int
main(int argc, char *argv[])
{
	cleanup(lua_closep) lua_State *L = NULL;
	kluad_context storage = {0};
	kluad_context *ctx = &storage;
	int rv;

#if 0
	extern char *malloc_options;
	malloc_options = "GJS";
#endif

	struct option longopts[] = {
		{"dev", no_argument, &ctx->is_dev, 1},
		{0},
	};

	while((rv = getopt_long(argc, argv, "", longopts, NULL)) != -1){
		switch(rv){
			case 0:
				break;
			default:
				usage();
		}
	}

	argc -= optind;
	argv += optind;

	if(argc != 1)
		usage();

	if((rv = lua_setup(&L, argv[0])) < 0)
		kutil_errx(NULL, NULL, "lua_setup: %s", strerror(-rv));

	if(luaL_dostring(L, "require('boot').run()") != LUA_OK){
		kutil_errx(NULL, NULL, "boot: %s", lua_tostring(L, -1));
		lua_pop(L, 1);
	}

	return 0;
}

