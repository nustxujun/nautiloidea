require("class")

Dispatcher = class("Dispatcher")

function Dispatcher:ctor()
    self.events = {}
end

function create_events_container()
    local t = {}
    setmetatable(t, {__mode = "k"});
    return t
end

function Dispatcher:add(event, key, callback)
    self.events[event] = self.events[event] or create_events_container()
    self.events[event][key] = callback
end

function Dispatcher:notify(event, ...)
    local events = self.events[event]
    if events then 
        for k,v in pairs(events) do 
            v(event, ...)
        end
    end
end
