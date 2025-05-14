local string = require("string")

local http = {}

local status = {
	okay = "200 OK",
	notfound = "404 Not Found",
	ise = "500 Internal Server Error",
}

http.status = status

local mimetypes = {
	textplain = "text/plain",
	texthtml = "text/html",
	textcss = "text/css",
	textjs = "text/javascript",

	gif = "image/gif",

	appjson = "application/json",
	binary = "application/octet-stream",
}

http.mimetypes = mimetypes

local mimemap = {
	css = http.mimetypes.textcss,
	html = http.mimetypes.texthtml,
	gif = http.mimetypes.gif
}

http.mimemap = mimemap

http.mimetype = function(name)
	local ext = string.match(name, ".(%a+)$")
	if ext then
		local mime = http.mimemap[ext]
		if mime then
			return mime
		end
	end

	return http.mimetypes.binary
end

local headers = {
	contenttype = "Content-Type",
}

http.headers = headers

return http

