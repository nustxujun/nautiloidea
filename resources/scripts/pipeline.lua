

require("class")

Pipeline = class("Pipeline")

function Pipeline:ctor(name)
    self.name = name
    self.pipeline = render.Pipeline.new()
    self.passes = {}
    self.is_dirty = true
end

function Pipeline:execute()
    if self.is_dirty then 
        self:refresh()
    end
    self.pipeline:execute()
end

function Pipeline:reset()
    self.pipeline.reset()
    self.passes = {}
end

function Pipeline:dirty()
    self.is_dirty = true
end

function Pipeline:refresh()
    p = self.pipeline
    p:reset()
    for k,v in pairs(self.passes) do 
        p:add_pass(v.name, v:get_render_pass())
    end
    self.is_dirty = false
end

function Pipeline:push_back(pass)
    self:insert(#self.passes + 1, pass)
end

function Pipeline:insert(index, pass)
    table.insert(self.passes, index, pass)
    pass.pipeline = self
end

function Pipeline:pop_back()
    self:erase(#self.passes)
end

function Pipeline:erase(index)
    self.passes[index].pipeline = nil
    table.erase(self.passes, index)
end

function Pipeline:get(index)
    return self.passes[index]
end


PipelineQueue = class("PipelineQueue")
function PipelineQueue:ctor()
    self.pipelines = {}
    self.ordered_pipelines = {}
end

function PipelineQueue:create_pipeline(name)
    local p = Pipeline(name)
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
    local pipelines = self.pipelines
    self.ordered_pipelines = {}
    for k,v in pairs(p) do 
        local pipeline = pipelines[v]
        assert(pipeline ~= nil, v .. " is a exsited pipeline.")
        self.ordered_pipelines[k] = pipeline
    end
end

function PipelineQueue:get_ordered_pipelines()
    tmp = {}
    for k,v in pairs(self.ordered_pipelines) do 
        tmp[k] = v.name
    end
    return tmp;
end

