
local pipelines = {}
local ordered_pipelines = {}

function create_pipeline(name)
    local p = render.Pipeline.new()
    pipelines[name] = p
    return p
end

function destroy_pipeline(name)
    pielines[name] = nil
end

function get_pipeline(name)
    return pipelines[name]
end

function execute()
    if #ordered_pipelines == 0 then 
        return 
    end
    for k,v in pairs(ordered_pipelines) do 
        v:execute()
    end
end

function set_ordered_pipelines(p)
    ordered_pipelines = {}
    for k,v in pairs(p) do 
        ordered_pipelines[k] = v
    end
end

function get_ordered_pipelines()
    tmp = {}
    for k,v in pairs(ordered_pipelines) do 
        tmp[k] = v
    end
    return tmp;
end

return {
    create_pipeline = create_pipeline,
    destroy_pipeline = destroy_pipeline,
    execute = execute,
    set_ordered_pipelines = set_ordered_pipelines,
    get_ordered_pipelines = get_ordered_pipelines,
}
