local resource_system = require("resource_system")

function register_resource(cls, ...)
	local exts = {...}
	for k,v in pairs(exts) do 
		resource_system.register_resource(v, cls)
	end
end

local ScriptResource = class("ScriptResource", resource_system.Resource)
ScriptResource.type = "Script"
register_resource(ScriptResource, "lua")

function ScriptResource:ctor(path)
	self.path = path:path()
	self.filename= path:filename()
	self.ext = path:extension()
	print ("create ", self.filename)
end
