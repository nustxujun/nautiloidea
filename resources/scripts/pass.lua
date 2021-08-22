Pass = class("Pass")


function Pass:ctor()
	self.resources = {}
end

function Pass:get_render_pass()
	assert(false, "pure virtual function")
end

function Pass:check()
end

function Pass:bind_resource(name, res)
	self.resources[name] = res
end

function Pass:get_resource(name)
	return self.resources[name]
end

ScenePass = class("ScenePass", Pass)

function ScenePass:get_render_pass()
	self:check()
	return render.pipeline_operation.render_scene(self.get_resource("rt"),self.get_resource('ds'))
end

function ScenePass:check()
	assert(self.get_resource("rt") and self.get_resource("ds"))
end


GUIPass = class("GUIPass", Pass)

function GUIPass:get_render_pass()
	self:check()
	return render.pipeline_operation.render_ui(self.get_resource("rt"))
end