#include <string.h>

#include <lua.h>
#include <lauxlib.h>

#include <kcgi.h>

#include <err.h>

#define FCGI_MT "fcgi_mt"
#define KREQ_MT "kreq_mt"

struct reqwrap {
	int done;
	struct kreq r;
};

static void
reqnotdone(lua_State *L, struct reqwrap *r)
{
	if(r->done)
		luaL_error(L, "request used after free");
}

static int
fcgi_parse(lua_State *L)
{
	enum kcgi_err err;
	struct kfcgi **fcgi;
	struct reqwrap *r;

	fcgi = luaL_checkudata(L, 1, FCGI_MT);

	r = lua_newuserdata(L, sizeof(struct reqwrap));
	memset(r, 0, sizeof(struct reqwrap));
	luaL_setmetatable(L, KREQ_MT);

	err = khttp_fcgi_parse(*fcgi, &r->r);
	if(err == KCGI_EXIT)
		return 0;

	if(err != KCGI_OK)
		luaL_error(L, "khttp_fcgi_parse: %s", kcgi_strerror(err));

	return 1;
}

static int
fcgi_get_fd(lua_State *L)
{
	struct kfcgi **fcgi;
	fcgi = luaL_checkudata(L, 1, FCGI_MT);
	
	lua_pushinteger(L, khttp_fcgi_getfd(*fcgi));
	return 1;
}

static int
fcgi_gc(lua_State *L)
{
	struct kfcgi **fcgi = luaL_checkudata(L, 1, FCGI_MT);

	khttp_fcgi_free(*fcgi);

	return 0;
}


static int
kreq_head(lua_State *L)
{
	enum kcgi_err err;
	const char *key, *value;
	struct reqwrap *r;

	r = luaL_checkudata(L, 1, KREQ_MT);
	reqnotdone(L, r);

	key = luaL_checkstring(L, 2);
	value = luaL_checkstring(L, 3);

	err = khttp_head(&r->r, key, "%s", value);
	if(err != KCGI_OK)
		luaL_error(L, "khttp_head: %s", kcgi_strerror(err));

	return 0;
}

static int
kreq_body(lua_State *L)
{
	enum kcgi_err err;
	struct reqwrap *r;

	r = luaL_checkudata(L, 1, KREQ_MT);
	reqnotdone(L, r);

	err = khttp_body(&r->r);
	if(err != KCGI_OK)
		luaL_error(L, "khttp_body: %s", kcgi_strerror(err));

	return 0;
}

static int
kreq_puts(lua_State *L)
{
	enum kcgi_err err;
	const char *str;
	struct reqwrap *r;

	r = luaL_checkudata(L, 1, KREQ_MT);
	reqnotdone(L, r);

	str = luaL_checkstring(L, 2);

	err = khttp_puts(&r->r, str);
	if(err != KCGI_OK)
		luaL_error(L, "khttp_puts: %s", kcgi_strerror(err));

	return 0;
}

static int
kreq_write(lua_State *L)
{
	enum kcgi_err err;
	const char *str;
	size_t len;
	struct reqwrap *r;

	r = luaL_checkudata(L, 1, KREQ_MT);
	reqnotdone(L, r);

	luaL_checktype(L, 2, LUA_TSTRING);
	str = lua_tolstring(L, 2, &len);

	err = khttp_write(&r->r, str, len);
	if(err != KCGI_OK)
		luaL_error(L, "khttp_write: %s", kcgi_strerror(err));

	return 0;
}

static int
kreq_free(lua_State *L)
{
	struct reqwrap *r;

	r = luaL_checkudata(L, 1, KREQ_MT);
	reqnotdone(L, r);

	khttp_free(&r->r);

	r->done = 1;

	return 0;
}

static int
kreq_fullpath(lua_State *L)
{
	struct reqwrap *r;

	r = luaL_checkudata(L, 1, KREQ_MT);
	reqnotdone(L, r);

	lua_pushstring(L, r->r.fullpath);

	return 1;
}

static int
kreq_host(lua_State *L)
{
	struct reqwrap *r;

	r = luaL_checkudata(L, 1, KREQ_MT);
	reqnotdone(L, r);

	lua_pushstring(L, r->r.host);

	return 1;
}

static int
kreq_remote(lua_State *L)
{
	struct reqwrap *r;

	r = luaL_checkudata(L, 1, KREQ_MT);
	reqnotdone(L, r);

	lua_pushstring(L, r->r.remote);

	return 1;
}

static int
kreq_method(lua_State *L)
{
	struct reqwrap *r;

	r = luaL_checkudata(L, 1, KREQ_MT);
	reqnotdone(L, r);

	lua_pushstring(L, kmethods[r->r.method]);

	return 1;
}

static int
kreq_headers(lua_State *L)
{
	struct reqwrap *r;
	struct khead *kh;
	size_t i;

	r = luaL_checkudata(L, 1, KREQ_MT);
	reqnotdone(L, r);

	lua_createtable(L, 0, r->r.reqsz);

	for(i = 0; i < r->r.reqsz; i++){
		kh = &r->r.reqs[i];
		lua_pushstring(L, kh->key);
		lua_pushstring(L, kh->val);
		lua_settable(L, -3);
	}

	return 1;
}

static int
kreq_cookies(lua_State *L)
{
	struct reqwrap *r;
	struct kpair *kp;
	size_t i;

	r = luaL_checkudata(L, 1, KREQ_MT);
	reqnotdone(L, r);

	lua_createtable(L, 0, r->r.cookiesz);

	for(i = 0; i < r->r.cookiesz; i++){
		kp = &r->r.cookies[i];
		lua_pushstring(L, kp->key);
		lua_pushstring(L, kp->val);
		lua_settable(L, -3);
	}

	return 1;
}

static int
kreq_fields(lua_State *L)
{
	struct reqwrap *r;
	struct kpair *kp;
	size_t i;

	r = luaL_checkudata(L, 1, KREQ_MT);
	reqnotdone(L, r);

	lua_createtable(L, 0, r->r.fieldsz);

	for(i = 0; i < r->r.fieldsz; i++){
		kp = &r->r.fields[i];
		lua_pushstring(L, kp->key);
		lua_pushstring(L, kp->val);
		lua_settable(L, -3);
	}

	return 1;
}

static int
kreq_gc(lua_State *L)
{
	struct reqwrap *r = luaL_checkudata(L, 1, KREQ_MT);

	if(!r->done)
		khttp_free(&r->r);

	return 0;
}

static const luaL_Reg kreq_meta[] = {
	{"head",	kreq_head},
	{"body",	kreq_body},
	{"puts",	kreq_puts},
	{"write",	kreq_write},
	{"free",	kreq_free},

	{"fullpath",	kreq_fullpath},
	{"host",	kreq_host},
	{"remote",	kreq_remote},
	{"method",	kreq_method},
	{"headers",	kreq_headers},
	{"cookies",	kreq_cookies},
	{"fields",	kreq_fields},

	{"__gc",	kreq_gc},
	{0, 0}
};

static const luaL_Reg fcgi_meta[] = {
	{"parse",	fcgi_parse},
	{"fd",		fcgi_get_fd},

	/* metamethods */
	{"__gc",	fcgi_gc},
	{0, 0}
};

static int init(lua_State* L)
{
	struct kfcgi **fcgi;

	fcgi = (struct kfcgi**) lua_newuserdata(L, sizeof(struct kfcgi*));
	if(khttp_fcgi_init(fcgi, NULL, 0, NULL, 0, 0) != KCGI_OK)
		luaL_error(L, "khttp_fcgi_init failed");

	luaL_setmetatable(L, FCGI_MT);
	
	return 1;
}

static luaL_Reg const kcgilib[] = {
	{ "init", init},
	{ 0, 0 }
};

int
luaopen_kcgi(lua_State* L)
{
	luaL_newlib(L, kcgilib);

	luaL_newmetatable(L, FCGI_MT);
	luaL_setfuncs(L, fcgi_meta, 0);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);

	luaL_newmetatable(L, KREQ_MT);
	luaL_setfuncs(L, kreq_meta, 0);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);

	return 1;
}
