#include <sys/queue.h>
#include <string.h>
#include <errno.h>
#include <imsg.h>

#include <lua.h>
#include <lauxlib.h>

#define IMSGBUF_MT "imsgbuf_mt"
#define IMSG_MT "imsg_mt"

static int
lua_imsg_data(lua_State *L)
{
	struct imsg *msg = luaL_checkudata(L, 1, IMSG_MT);
	struct ibuf ibuf;
	size_t left;
	char *p;
	luaL_Buffer b;

	luaL_buffinit(L, &b);

	if(imsg_get_ibuf(msg, &ibuf) < 0)
		luaL_error(L, "imsg_get_ibuf");

	while((left = ibuf_size(&ibuf)) > 0){
		left = left > LUAL_BUFFERSIZE ? LUAL_BUFFERSIZE : left;
		p = luaL_prepbuffer(&b);
		if(ibuf_get(&ibuf, p, left) < 0)
			luaL_error(L, "ibuf_get: %s", strerror(errno));

		luaL_addsize(&b, left);
	}

	luaL_pushresult(&b);
	return 1;
}

static int
lua_imsg_fd(lua_State *L)
{
	struct imsg *msg = luaL_checkudata(L, 1, IMSG_MT);
	lua_pushinteger(L, imsg_get_fd(msg));
	return 1;
}

static int
lua_imsg_id(lua_State *L)
{
	struct imsg *msg = luaL_checkudata(L, 1, IMSG_MT);
	lua_pushinteger(L, imsg_get_id(msg));
	return 1;
}

static int
lua_imsg_len(lua_State *L)
{
	struct imsg *msg = luaL_checkudata(L, 1, IMSG_MT);
	lua_pushinteger(L, imsg_get_len(msg));
	return 1;
}

static int
lua_imsg_pid(lua_State *L)
{
	struct imsg *msg = luaL_checkudata(L, 1, IMSG_MT);
	lua_pushinteger(L, imsg_get_pid(msg));
	return 1;
}

static int
lua_imsg_type(lua_State *L)
{
	struct imsg *msg = luaL_checkudata(L, 1, IMSG_MT);
	lua_pushinteger(L, imsg_get_type(msg));
	return 1;
}

static int
lua_imsg_gc(lua_State *L)
{
	struct imsg *msg = luaL_checkudata(L, 1, IMSG_MT);

	imsg_free(msg);
	return 0;
}

static const luaL_Reg imsg_meta[] = {
	{"data", 	lua_imsg_data},
	{"fd",	 	lua_imsg_fd},
	{"id", 		lua_imsg_id},
	{"len", 	lua_imsg_len},
	{"pid", 	lua_imsg_pid},
	{"type", 	lua_imsg_type},
	{"__gc",	lua_imsg_gc},
	{0, 0}
};

/* :compose(type, id, pid, fd, buf) */
static int
lua_imsgbuf_compose(lua_State *L)
{
	struct imsgbuf *im = luaL_checkudata(L, 1, IMSGBUF_MT);
	int typ = luaL_checkinteger(L, 2);
	int id = luaL_checkinteger(L, 3);
	int pid = luaL_checkinteger(L, 4);
	int fd = luaL_checkinteger(L, 5);
	size_t sz;
	const char *buf = luaL_checklstring(L, 6, &sz);

	if(imsg_compose(im, typ, id, pid, fd, buf, sz) < 0)
		luaL_error(L, "imsg_compose");

	return 0;
}

static int
lua_imsgbuf_write(lua_State *L)
{
	struct imsgbuf *im = luaL_checkudata(L, 1, IMSGBUF_MT);

	if(imsgbuf_write(im) < 0)
		luaL_error(L, "imsgbuf_write: %s", strerror(errno));

	return 0;
}

static int
lua_imsgbuf_flush(lua_State *L)
{
	struct imsgbuf *im = luaL_checkudata(L, 1, IMSGBUF_MT);

	if(imsgbuf_flush(im) < 0)
		luaL_error(L, "imsgbuf_flush: %s", strerror(errno));

	return 0;
}

static int
lua_imsgbuf_read(lua_State *L)
{
	struct imsgbuf *im = luaL_checkudata(L, 1, IMSGBUF_MT);

	switch(imsgbuf_read(im)){
	case 0:
		luaL_error(L, "imsgbuf_read: closed");
	case -1:
		luaL_error(L, "imsgbuf_read: %s", strerror(errno));
	}

	return 0;
}

static int
lua_imsgbuf_get(lua_State *L)
{
	struct imsgbuf *im = luaL_checkudata(L, 1, IMSGBUF_MT);
	struct imsg *msg;
	ssize_t rv;

	msg = lua_newuserdata(L, sizeof(*msg));
	luaL_setmetatable(L, IMSG_MT);

	switch((rv = imsg_get(im, msg))){
	case 0:
		luaL_error(L, "imsg_get: %s", strerror(EBADMSG));
	case -1:
		luaL_error(L, "imsg_get: %s", strerror(errno));
	}

	return 1;
}

static int
lua_imsgbuf_gc(lua_State *L)
{
	struct imsgbuf *im = luaL_checkudata(L, 1, IMSGBUF_MT);

	(void) im;

	/* XXX: close? would need to store fd */
	return 0;
}

static const luaL_Reg imsgbuf_meta[] = {
	{"compose",	lua_imsgbuf_compose},
	{"write",	lua_imsgbuf_write},
	{"flush",	lua_imsgbuf_flush},
	{"read",	lua_imsgbuf_read},
	{"get",		lua_imsgbuf_get},
	{"__gc",	lua_imsgbuf_gc},
	{0, 0}
};

static int
lua_imsgbuf_new(lua_State *L)
{
	struct imsgbuf *im;
	int fd = luaL_checkinteger(L, 1);

	im = lua_newuserdata(L, sizeof(*im));
	luaL_setmetatable(L, IMSGBUF_MT);

	if(imsgbuf_init(im, fd) < 0)
		luaL_error(L, "imsgbuf_init");

	return 1;
}

static luaL_Reg const eventlib[] = {
	{ "new", lua_imsgbuf_new},
	{ 0, 0 }
};

int
luaopen_imsg(lua_State* L)
{
	luaL_newlib(L, eventlib);

	luaL_newmetatable(L, IMSGBUF_MT);
	luaL_setfuncs(L, imsgbuf_meta, 0);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);

	luaL_newmetatable(L, IMSG_MT);
	luaL_setfuncs(L, imsg_meta, 0);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);

	return 1;
}
