local string = require("string")

local openbsd = require("openbsd")
local event = require("event")
local router = require("router")
local http = require("http")
local template = require("template")

-- luajit itself uses prot_exec.
openbsd.pledge("stdio unix recvfd sendfd proc prot_exec")

local evb = event.init()
local rt = router.new()

local okay = http.status.okay
local txtplain = {[http.headers.contenttype] = http.mimetypes.textplain}

local cnt = 0

local function cb()
	cnt = cnt + 1
	--native.settimeout(1000, cb)
	return true
end

cntevt = evb:add(-1, "tp", cb, 10000)

local index = function(req)
	return okay, txtplain, "tis is the index milord\n"
end

rt:handle("^$", index)
rt:handle("^/$", index)

local top_template = template.compile[[
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<title>Status</title>
<link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/bulma@1.0.4/css/bulma.min.css">
</head>
<body>
<main>
<nav class="navbar is-spaced is-info">
<a href="<% path %>" class="navbar-item">Status</a>
<a href="/something" class="navbar-item">Something else</a>
</nav>
<div class="container">
<%= render(inner, getfenv()) %>
<!-- TODO -->
</div>
</main>
</body>
</html>
]]

local inner_template = template.compile[[
<h2>Hello, <% world %>!</h2>
]]

local render = function(t, arg)
	assert(t)
	local out = {}
	template.print(t, arg, function(s) table.insert(out, s) end)
	return table.concat(out, "")
end

rt:handle("^/status", function(req, match)
	local arg = {
		path = match,
		render = render,
		inner = inner_template,
		world = "world",
	}

	return okay, {[http.headers.contenttype] = http.mimetypes.texthtml}, render(top_template, arg)
end)

rt:handle("^/cnt", function(req, match)
	return okay, txtplain, tostring(cnt)
end)

rt:handle("^/pid", function(req, match)
	return okay, txtplain, tostring(openbsd.getppid())
end)

rt:handle("^/headers", function(req, match)
	local s = "method .. " .. req:method() .. "\n"
	s = s ..  "path   .. " .. req:fullpath() .. "\n"
	s = s ..  "host   .. " .. req:host() .. "\n"
	s = s ..  "remote .. " .. req:remote() .. "\n\n"

	for k,v in pairs(req:headers()) do
		s = s .. k .. " .. " .. v .. "\n"
	end
	return okay, txtplain, s
end)

rt:handle("^/htdocs/.+", function(req, match)
	local mime = http.mimetype(match)
	local f = io.open(match)
	if not f then
		return rt:notfound()
	end

	local b = f:read("*a")
	f:close()

	return okay, {[http.headers.contenttype] = mime}, b
end)

rt:handle("^/(%a+)", function(req, match)
	local f = string.format("Welcome to %s!\n", match)
	return okay, txtplain, f
end)

