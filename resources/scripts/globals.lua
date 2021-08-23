
require("dispatcher")

local dispatcher = Dispatcher()

EVENT_WINDOW_RESIZE = "window_resize"

core.window_resize_callback = function ()
	dispatcher:notify(EVENT_WINDOW_RESIZE)
end

function bind_event_window_resize(key, callback)
	dispatcher:add(EVENT_WINDOW_RESIZE, key, callback)
end

return _ENV
