-- luajit bytecode <> c array

local io = assert(require('io'))
local table = assert(require('table'))

local fname = table.remove(arg, 1)

local f = assert(io.open(fname, "w"))

f:write([[
#include <stddef.h>

#include "bytecode.h"

]])

for _, name in ipairs(arg) do
	local lname = string.gsub(name, ".lua$", "")
	-- strip path
	lname = string.gsub(lname, ".+/", "")
	f:write(string.format([[#include "%s_bytecode.h"]], lname), "\n")
end

f:write([[

bytecode bytecode_list[] = {
]])

for _, name in ipairs(arg) do
	local lname = string.gsub(name, ".lua$", "")
	-- strip path
	lname = string.gsub(lname, ".+/", "")
	f:write(string.format([[  { luaJIT_BC_%s, luaJIT_BC_%s_SIZE, "%s" },]], lname, lname, lname), "\n")
end

f:write([[
  {0},
};
]])

