#include <sys/socket.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/un.h>
#include <uvm/uvmexp.h>

#include <string.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include <lua.h>
#include <lauxlib.h>

static int
lerrno(lua_State *L, const char *tag)
{
	return luaL_error(L, "%s: %s", tag, strerror(errno));
}

static int
openbsd_pledge(lua_State *L)
{
	const char *promises = lua_tolstring(L, 1, NULL);
	const char *execpromises = lua_tolstring(L, 2, NULL);

	if(pledge(promises, execpromises) < 0)
		lerrno(L, "pledge");

	return 0;
}

static int
openbsd_getpid(lua_State *L)
{
	lua_pushinteger(L, getpid());
	return 1;
}

static int
openbsd_getppid(lua_State *L)
{
	lua_pushinteger(L, getpid());
	return 1;
}

static int
openbsd_kill(lua_State *L)
{
	pid_t pid;
	int sig;

	pid = luaL_checkint(L, 1);
	sig = luaL_checkint(L, 2);

	if(kill(pid, sig) < 0)
		lerrno(L, "kill");
	
	return 0;
}

static int
openbsd_socket(lua_State *L)
{
	int domain, type, protocol, fd;

	domain = luaL_checkint(L, 1);
	type = luaL_checkint(L, 2);
	protocol = luaL_checkint(L, 3);

	if((fd = socket(domain, type, protocol)) < 0)
		lerrno(L, "socket");

	lua_pushinteger(L, fd);

	return 1;
}

static int
openbsd_socketpair(lua_State *L)
{
	int domain, type, protocol, pair[2];

	domain = luaL_checkint(L, 1);
	type = luaL_checkint(L, 2);
	protocol = luaL_checkint(L, 3);

	if(socketpair(domain, type, protocol, pair) < 0)
		return lerrno(L, "socketpair");

	lua_pushinteger(L, pair[0]);
	lua_pushinteger(L, pair[1]);
	
	return 2;
}

static int
openbsd_connect_unix(lua_State *L)
{
	int fd = luaL_checkint(L, 1);
	const char *path = luaL_checkstring(L, 2);
	struct sockaddr_un sun;

	memset(&sun, 0, sizeof(sun));
	sun.sun_family = AF_UNIX;

	strlcpy(sun.sun_path, path, sizeof(sun.sun_path));

	if(connect(fd, (struct sockaddr*)&sun, sizeof(sun)) < 0)
		lerrno(L, "connect");

	return 0;
}

static int
openbsd_close(lua_State *L)
{
	int fd;

	fd = luaL_checkint(L, 1);

	close(fd);

	return 0;
}

#define pfield(struc, field) \
	lua_pushstring(L, #field); \
	lua_pushstring(L, struc.field); \
	lua_settable(L, -3);

static int
openbsd_uname(lua_State *L)
{
	struct utsname u;

	if(uname(&u) < 0)
		return lerrno(L, "uname");

	lua_createtable(L, 0, 5);

	pfield(u, sysname);
	pfield(u, nodename);
	pfield(u, release);
	pfield(u, version);
	pfield(u, machine);

	return 1;
}

#define pfieldint(struc, field) \
	lua_pushstring(L, #field); \
	lua_pushnumber(L, struc.field); \
	lua_settable(L, -3);

static int
openbsd_uvmexp(lua_State *L)
{
	int mib[2] = {CTL_VM, VM_UVMEXP};
	struct uvmexp u;
	size_t ul = sizeof(u);

	if(sysctl(mib, sizeof(mib)/sizeof(mib[0]), &u, &ul, NULL, 0) < 0)
		return lerrno(L, "sysctl");

	lua_newtable(L);

	pfieldint(u, pagesize);
	pfieldint(u, npages);
	pfieldint(u, free);
	pfieldint(u, active);
	pfieldint(u, inactive);
	pfieldint(u, paging);
	pfieldint(u, wired);

	return 1;
}

static int
openbsd_loadavg(lua_State *L)
{
	double loadavg[3];

	getloadavg(loadavg, 3);

	lua_pushnumber(L, loadavg[0]);
	lua_pushnumber(L, loadavg[1]);
	lua_pushnumber(L, loadavg[2]);

	return 3;
}

static luaL_Reg const openbsdlib[] = {
	{"pledge",	openbsd_pledge},
	{"getpid",	openbsd_getpid},
	{"getppid",	openbsd_getppid},
	{"kill",	openbsd_kill},

	{"socket",	openbsd_socket},
	{"socketpair",	openbsd_socketpair},
	{"connect_unix",openbsd_connect_unix},
	{"close",	openbsd_close},
	{"uname",	openbsd_uname},
	{"uvmexp",	openbsd_uvmexp},
	{"loadavg",	openbsd_loadavg},
	{0},
};

#define putint(L, something) \
	lua_pushinteger(L, something); \
	lua_setfield(L, -2, #something);

int
luaopen_openbsd(lua_State* L)
{
	luaL_newlib(L, openbsdlib);

	/* sockets etc */
	putint(L, AF_UNSPEC);
	putint(L, AF_UNIX);

	putint(L, SOCK_STREAM);
	putint(L, SOCK_CLOEXEC);
	putint(L, SOCK_NONBLOCK);

	putint(L, PF_UNSPEC);

	putint(L, SIGTERM);
	putint(L, SIGHUP);
	return 1;
}

