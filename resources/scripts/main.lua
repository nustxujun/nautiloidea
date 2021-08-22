
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

local rt, ds;

local pipeline_queue = PipelineQueue()
local pipeline = pipeline_queue:create_pipeline("main")
pipeline_queue:set_ordered_pipelines({pipeline})

core.window_resize_callback = function ()
    local w,h = core.get_window_size()
    rt = render.create_resource(1, w,h, 28)
    -- ds = render.create_resource(1, w,h, 28)
    pipeline:reset()

    -- pipeline:add_pass("render scene", render.pipeline_operation.render_scene(rt,ds))
    pipeline:add_pass("render ui", render.pipeline_operation.render_ui(rt))
    pipeline:add_pass("present", render.pipeline_operation.present(rt))

end
core.window_resize_callback()

core.update_callback = function ()
    pipeline_queue:execute()
    Gui.tick()
end
