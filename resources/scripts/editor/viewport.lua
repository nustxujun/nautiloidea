require("class")
require("pipeline")
local Globals = require("globals")
local Gui = require("gui")



local Viewport = class("Viewport")
local pipeline_queue = PipelineQueue()
local viewports = {}
setmetatable(viewports, {__mode = "k"});

Globals.bind_event_update(Viewport, function()
	local list = {}
	for k,v in pairs(viewports) do 
		local win = v.window
		if (win.visible and win.opened.value and win.is_drawn) then 
			list[#list + 1] = v.pipeline
		end
	end
	pipeline_queue:set_ordered_pipelines(list)
end)
Globals.add_pipeline_queue("viewports", pipeline_queue)


function Viewport:ctor(name)
	viewports[#viewports + 1] = self
	self.window = Window(name, true)
	Gui.Root:add_child(self.window)

	self.pipeline = Pipeline(name)
end

function Viewport:show()
	self.window.opened.value = true
end

return Viewport