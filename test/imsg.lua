local openbsd = require("openbsd")
local imsg = require("imsg")

local p0, p1 = openbsd.socketpair(openbsd.AF_UNIX, openbsd.SOCK_STREAM, openbsd.PF_UNSPEC)

local buf0, buf1 = imsg.new(p0), imsg.new(p1)

local typ, id, payload = 42, 69, "hello, world!"

buf0:compose(typ, id, 0, -1, payload)
buf0:write()

buf1:read()
local msg = buf1:get()

assert(msg:type() == typ)
assert(msg:id() == id)
assert(msg:data() == payload)

