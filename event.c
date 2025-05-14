#include <string.h>
#include <errno.h>

#include <event.h>

#include <lua.h>
#include <lauxlib.h>

#include <err.h>

#define EVENT_BASE_MT "event_base_mt"
#define EVENT_MT "event_mt"

typedef struct event_thunk {
	lua_State *L;
	struct event ev;
	int selfref;
	int funcref;
	struct timeval tv;
} event_thunk;

static struct timeval*
maybetv(event_thunk *e)
{
	if(e->tv.tv_sec > 0 && e->tv.tv_sec > 0)
		return &e->tv;

	return NULL;
}

static int
lua_event_gc(lua_State *L)
{
	event_thunk *et = luaL_checkudata(L, 1, EVENT_MT);

	event_del(&et->ev);
	luaL_unref(L, LUA_REGISTRYINDEX, et->funcref);

	return 0;
}

static const luaL_Reg event_meta[] = {
	{"__gc",	lua_event_gc},
	{0, 0}
};

static int
lua_event_dispatch(lua_State *L)
{
	struct event_base *base = luaL_checkudata(L, 1, EVENT_BASE_MT);
	int rc;

	/* a lie */
	(void) base;

	rc = event_dispatch();
	if(rc)
		luaL_error(L, "event_base_dispatch: %d (%s)", rc, strerror(errno));

	return 0;
}

static void
evmask_to_str(short events, char *repr)
{
	char *p = repr;

	if(events & EV_READ) *p++ = 'r';
	if(events & EV_WRITE) *p++ = 'w';
	if(events & EV_TIMEOUT) *p++ = 't';
	if(events & EV_SIGNAL) *p++ = 's';
	if(events & EV_PERSIST) *p++ = 'p';
	*p = '\0';
}

/* call bool = f(struct event, fd, mask) */
static void
lua_event_fire(int fd, short events, void *v)
{
	event_thunk *et = v;
	char evstr[6];

	evmask_to_str(events, evstr);

	/* func */
	lua_rawgeti(et->L, LUA_REGISTRYINDEX, et->funcref);

	/* event */
	lua_rawgeti(et->L, LUA_REGISTRYINDEX, et->selfref);

	/* fd */
	lua_pushnumber(et->L, fd);

	/* mask */
	lua_pushstring(et->L, evstr);

	/* i suspect this is unsafe across libevent, but i'm not sure */
	lua_call(et->L, 3, 1);

	if(lua_toboolean(et->L, -1)){
		event_add(&et->ev, maybetv(et));
	} else {
		/* cleanup registry handle */
		luaL_unref(et->L, LUA_REGISTRYINDEX, et->selfref);
	}
}

static short
str_to_evmask(const char *s)
{
	short mask = 0;

	if(strchr(s, 'r')) mask |= EV_READ;
	if(strchr(s, 'w')) mask |= EV_WRITE;
	if(strchr(s, 't')) mask |= EV_TIMEOUT;
	if(strchr(s, 's')) mask |= EV_SIGNAL;
	if(strchr(s, 'p')) mask |= EV_PERSIST;

	return mask;
}

/* add(self, fd, mask, cb, timeout)
 * 	cb(event, fd, mask)
 */
static int
lua_event_add(lua_State *L)
{
	struct event_base *evb;
	int fd, funcref;
	short evmask;
	long ms = 0;
	const char *evstr;
	event_thunk *et;

	/* ignored, since it's really global */
	evb = luaL_checkudata(L, 1, EVENT_BASE_MT);
	(void) evb;

	fd = luaL_checkint(L, 2);
	evstr = luaL_checkstring(L, 3);
	evmask = str_to_evmask(evstr);
	luaL_checktype(L, 4, LUA_TFUNCTION);
	/* func ref at top */
	lua_pushvalue(L, 4);
	funcref = luaL_ref(L, LUA_REGISTRYINDEX);

	if(lua_isnumber(L, 5)){
		ms = luaL_checklong(L, 5);
	}

	et = lua_newuserdata(L, sizeof(*et));
	luaL_setmetatable(L, EVENT_MT);

	et->L = L;
	event_set(&et->ev, fd, evmask, lua_event_fire, et);

	lua_pushvalue(L, -1);
	et->selfref = luaL_ref(L, LUA_REGISTRYINDEX);

	et->funcref = funcref;

	et->tv.tv_sec = ms/1000;
	et->tv.tv_usec = (ms%1000) * 1000;

	event_add(&et->ev, maybetv(et));

	return 1;
}

static int
lua_event_break(lua_State *L)
{
	struct event_base *evb;

	/* ignored, since it's really global */
	evb = luaL_checkudata(L, 1, EVENT_BASE_MT);
	(void) evb;

	event_loopbreak();

	return 0;
}

static const luaL_Reg event_base_meta[] = {
	{"dispatch", 	lua_event_dispatch},
	{"add", 	lua_event_add},
	{"break",	lua_event_break},
	{0, 0}
};

static int init(lua_State* L)
{
	static struct event_base *evb, **p;

	if(!evb){
		evb = event_init();
		lua_pushlightuserdata(L, evb);
		p = lua_newuserdata(L, sizeof(struct event_base*));
		*p = evb;
		luaL_setmetatable(L, EVENT_BASE_MT);
		/* dup for return */
		lua_settable(L, LUA_REGISTRYINDEX);
	}

	lua_pushlightuserdata(L, evb);
	lua_gettable(L, LUA_REGISTRYINDEX);

	return 1;
}

static luaL_Reg const eventlib[] = {
	{ "init", init},
	{ 0, 0 }
};

int
luaopen_event(lua_State* L)
{
	luaL_newlib(L, eventlib);

	luaL_newmetatable(L, EVENT_BASE_MT);
	luaL_setfuncs(L, event_base_meta, 0);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);

	luaL_newmetatable(L, EVENT_MT);
	luaL_setfuncs(L, event_meta, 0);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);

	return 1;
}

