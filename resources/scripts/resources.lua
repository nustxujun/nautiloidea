local resource_system = require("resource_system")

function register_factory(cls, ...)
	local exts = {...}
	for k,v in pairs(exts) do 
		resource_system.register_factory(v, function (path)
			return cls(path)
		end)
	end
end

local ScriptResource = class("ScriptResource", resource_system.Resource)
register_factory(ScriptResource, "lua")

function ScriptResource:ctor(path)
	self.path = path:path()
	self.filename= path:filename()
	self.ext = path:extension()
	print ("create ", self.filename)
end
