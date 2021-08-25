
require("dispatcher")

local dispatcher = Dispatcher()
local pipeline_queue_list = {}

EVENT_WINDOW_RESIZE = "window_resize"
EVENT_UPDATE = "update"

core.window_resize_callback = function ()
	dispatcher:notify(EVENT_WINDOW_RESIZE)
end

function bind_event_window_resize(key, callback)
	dispatcher:add(EVENT_WINDOW_RESIZE, key, callback)
end

core.update_callback = function ()
	for k,v in pairs(pipeline_queue_list) do 
		v:execute()
	end
	dispatcher:notify(EVENT_UPDATE)
end

function bind_event_update(key, callback)
	dispatcher:add(EVENT_UPDATE, key, callback)
end

function add_pipeline_queue(name, pq)
	pipeline_queue_list[name] = pq
end

function remove_pipeline_queue(name)
	pipeline_queue_list[name] = nil
end

return _ENV
