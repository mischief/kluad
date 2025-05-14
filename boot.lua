local os = require("os")
local debug = require("debug")

local openbsd = require("openbsd")
local kcgi = require("kcgi")
local event = require("event")

local router = require("router")

local boot = {}

boot.ise = function(self, r)
	r:head("Status", "500 Internal Server Error")
	r:body()
	r:puts("Internal Server Error")
	r:free()
end

boot.handler = function(_, fd, mask)
	local r = boot.kcgi:parse()

	-- TODO: better logging for ISE
	status, err = xpcall(function() router.default:serve(r) end, debug.traceback)
	if not status then
		print(err)
		boot:ise(r)
	end
end

-- doesn't seem to do anything beyond kcgi's existing SIGTERM handling.
boot.sighandler = function(_, signal, mask)
	print("caught signal", signal)
	os.exit(0)
end

boot.run = function()
	boot.eventbase = event.init()
	boot.kcgi = kcgi.init()
	boot.cgifd = boot.kcgi:fd()
	boot.event = boot.eventbase:add(boot.cgifd, "rp", boot.handler)
	boot.sigterm = boot.eventbase:add(openbsd.SIGTERM, "sp", boot.sighandler)
	boot.eventbase:dispatch()
end

return boot

