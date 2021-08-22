

require("class")

Pipeline = class("Pipeline")

function Pipeline:ctor()
    self.pipeline = render.Pipeline.new()
    self.passes = {}
end

function Pipeline:execute()
    self.pipeline:execute()
end

function Pipeline:reset()
    self.pipeline.reset()
end

function Pipeline:refresh()
    p = self.pipeline
    p:reset()
    for k,v in pairs(self.passes) do 
        p:add_pass(v:get_render_pass())
    end
end


PipelineQueue = class("PipelineQueue")
function PipelineQueue:ctor()
    self.pipelines = {}
    self.ordered_pipelines = {}
end

function PipelineQueue:create_pipeline(name)
    local p = render.Pipeline.new()
    self.pipelines[name] = p
    return p
end

function PipelineQueue:destroy_pipeline(name)
    self.pielines[name] = nil
end

function PipelineQueue:get_pipeline(name)
    return self.pipelines[name]
end

function PipelineQueue:execute()
    if #self.ordered_pipelines == 0 then 
        return 
    end
    for k,v in pairs(self.ordered_pipelines) do 
        v:execute()
    end
end

function PipelineQueue:set_ordered_pipelines(p)
    self.ordered_pipelines = {}
    for k,v in pairs(p) do 
        self.ordered_pipelines[k] = v
    end
end

function PipelineQueue:get_ordered_pipelines()
    tmp = {}
    for k,v in pairs(self.ordered_pipelines) do 
        tmp[k] = v
    end
    return tmp;
end

