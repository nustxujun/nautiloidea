
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
local Globals = require("globals")

local rt = PassResource()
Globals.bind_event_window_resize(rt, function()
    local w,h = core.get_window_size()
    rt:reset(1,w,h,28)
end)
local w,h = core.get_window_size()
rt:reset(1,w,h,28)

local pipeline_queue = PipelineQueue()
Globals.add_pipeline_queue("main",pipeline_queue)
local pipeline = Pipeline("main")
pipeline_queue:set_ordered_pipelines({pipeline})

local uipass = GUIPass("ui pass")
local present = FinalPass("present")
pipeline:push_back(uipass)
pipeline:push_back(present)

uipass:bind_resource("rt", rt)
present:bind_resource("rt",rt)

Globals.bind_event_update(Gui, function ()
    Gui.tick()
end)

