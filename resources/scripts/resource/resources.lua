local resource_system = require("resource/resource_system")
local Resource = resource_system.Resource

function register_resource(cls, ...)
	local exts = {...}
	for k,v in pairs(exts) do 
		resource_system.register_resource(v, cls)
	end
end


-- ScriptResource -------------------------------------------

local ScriptResource = class("ScriptResource", Resource)
ScriptResource.type = "Script"
register_resource(ScriptResource, "lua")

function ScriptResource:ctor(path)
	self.super().ctor(self,path)
end


-- ModelResource -------------------------------------------
local ModelResource = class("ModelResource", Resource)
ModelResource.type = "Model"
register_resource(ModelResource, "fbx", "obj")

function ModelResource:ctor(path)
	self.super(Resource).ctor(self,path)
end

