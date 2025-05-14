local router = require("router")

local reqmt = {}
reqmt.fullpath = function(self)
	return self._path
end
reqmt.method = function(self)
	return self._method
end
reqmt.host = function(self)
	return self._host
end
reqmt.head = function(self, k, v)
	self._headers[k] = v
end
reqmt.body = function(self)
	self._body = true
end
reqmt.write = function(self, s)
	self._content = self._content or "" .. s
end
reqmt.free = function(self)
	self._free = true
end

local mkreq = function(pth, mth, host)
	local t = {
		_path = pth or "/",
		_method = mth or "GET",
		_host = host or "exmple.com",
		_headers = {}
	}
	return setmetatable(t, {__index=reqmt})
end

local checkcode = function(rt, req, code, content)
	rt:serve(req)
	assert(req._headers["Status"] == code)
	if content then
		assert(req._content == content)
	end
end

rt = router.new()

-- anything starting with foo
rt:handle("^/foo", function(req, match)
	return 999
end)

rt:handle("GET /baz/(%a+)", function(req, match)
	return 200, {}, match
end)

checkcode(rt, mkreq("/foo"), 999)

checkcode(rt, mkreq("/bar"), 404)

checkcode(rt, mkreq("/baz/quux", "GET"), 200, "quux")

