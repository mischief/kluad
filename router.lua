local router =  {}

local rtmt = {}

local splmeth = function(match)
	return string.match(match, "(%u+)%s+(.+)")
end

rtmt.handle = function(self, match, how)
	local t = {}

	if type(how) == "table" then
		t.handle = function(r, ...)
			how:serve(r, ...)
		end
	else
		t.handle = how
	end

	local meth, pth = splmeth(match)

	if meth then
		t.method = meth
		t.match = pth
	else
		t.match = match
	end

	table.insert(self.handlers, t)
	return self
end

rtmt.notfound = function(self)
	return 404, {}, "Not Found"
end

rtmt.serve = function(self, r)
	local found, code, headers, body
	local p = r:fullpath()
	local meth = r:method()
	local host = r:host()

	for i, m in ipairs(self.handlers) do
			local methmatch = m.method and m.method == meth or true
			local matches = {string.match(p, m.match)}
			if methmatch and #matches > 0 then
				found = true
				code, headers, body = m.handle(r, unpack(matches))
				break
			end
	end

	if not found then
		code, headers, body = self:notfound()
	end

	r:head("Status", code)

	if headers then
		for k,v in pairs(headers) do
			r:head(k,v)
		end
	end

	r:body()

	if body then
		r:write(body)
	end

	r:free()
end

router.new = function()
	local t = { handlers = {} }
	local rt = setmetatable(t, {__index=rtmt})
	if not router.default then
		router.default = rt
	end

	return rt
end

return router

