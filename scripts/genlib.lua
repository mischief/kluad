local io = assert(require('io'))
local table = assert(require('table'))

local fname = table.remove(arg, 1)

local f = assert(io.open(fname, "w"))

f:write([[
#include <lua.h>

#include "genlib.h"

]])

for _, name in ipairs(arg) do
	-- accident prevention
	assert(#name > 0)

	f:write(string.format([[extern int luaopen_%s(lua_State*);]], name), "\n")
end

f:write([[

genlib genlib_list[] = {
]])

for _, name in ipairs(arg) do
	f:write(string.format([[  { luaopen_%s, "%s" },]], name, name), "\n")
end

f:write([[
  {0},
};
]])

