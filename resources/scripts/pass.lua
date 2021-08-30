require("dispatcher")
local Globals = require("globals")
PassResource = class("PassResource")


EVENT_RESET = "event_reset"

function PassResource:ctor()
	self.dispatcher = Dispatcher()
	-- if event_resize_callback then 
	-- 	local this = self
	-- 	Globals.bind_event_window_resize(self,function()
	-- 		event_resize_callback(this)
	-- 	end)
	-- 	event_resize_callback(self)
	-- end
end

function PassResource:reset(type, width, height, format, clean_value)
	self.resource = render.create_resource(type, width, height, format,clean_value or {0,0,0,0})
	self.dispatcher:notify(EVENT_RESET)
end

function PassResource:get_device_resource()
	return self.resource
end

function PassResource:bind_reset_event(pipeline, callback)
	self.dispatcher:add(EVENT_RESET, pipeline, callback)
end

function PassResource:unbind_reset_event(callback)
	self.dispatcher:remove(EVENT_RESET, pipeline, callback)
end

function PassResource:get_gpu_descriptor_handle()
	return self.resource:get_gpu_descriptor_handle()
end

Pass = class("Pass")


function Pass:ctor(name)
	assert(name ~= nil)
	self.resources = {}
	self.name = name
end

function Pass:get_render_pass()
	assert(false, "pure virtual function")
end

function Pass:check()
end

function Pass:bind_resource(name, res)
	self.resources[name] = res
	local this = self
	res:bind_reset_event(self, function()
		this:dirty()
	end)
	self:dirty()
end

function Pass:dirty()
	if not self.pipeline then 
		return
	end
	self.pipeline:dirty();
end

function Pass:get_resource(name)
	return self.resources[name]
end

function Pass:get_device_resource(name)
	return self.resources[name]:get_device_resource()
end

ScenePass = class("ScenePass", Pass)

function ScenePass:get_render_pass()
	self:check()
	return render.pipeline_operation.render_scene(self:get_device_resource("rt"),self:get_device_resource('ds'))
end

function ScenePass:check()
	assert(self.get_resource("rt") and self.get_resource("ds"))
end


GUIPass = class("GUIPass", Pass)

function GUIPass:get_render_pass()
	self:check()
	return render.pipeline_operation.render_ui(self:get_device_resource("rt"))
end


FinalPass = class("FinalPass", Pass)

function FinalPass:get_render_pass()
	return render.pipeline_operation.present(self:get_device_resource("rt"))
end


ScenePass = class("ScenePass", Pass)

function ScenePass:set_root(root)
	self.root = root
end

function ScenePass:get_render_pass()
	return render.pipeline_operation.render_scene(self.root, self:get_device_resource("rt"), self:get_device_resource("ds"))
end