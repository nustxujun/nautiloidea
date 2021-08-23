
-- 

-- w,h = core.get_window_size()
-- local rt = render.create_resource(1, w,h, 28)
-- rt:test()
-- print (rt)
-- rt = nil
-- pass = render.pipeline_operation.render_scene(rt, rt)
-- p:add_pass("render scene",pass)

local Gui = require("gui")
require("editor/ui")
require("pipeline")
require("pass")

local rt = PassResource(function(rt)
    local w,h = core.get_window_size()
    rt:reset(1,w,h,28)
end)

local pipeline_queue = PipelineQueue()
local pipeline = pipeline_queue:create_pipeline("main")
pipeline_queue:set_ordered_pipelines({"main"})

local uipass = GUIPass("ui pass")
local present = FinalPass("present")
pipeline:push_back(uipass)
pipeline:push_back(present)

uipass:bind_resource("rt", rt)
present:bind_resource("rt",rt)


core.update_callback = function ()
    pipeline_queue:execute()
    Gui.tick()
end
