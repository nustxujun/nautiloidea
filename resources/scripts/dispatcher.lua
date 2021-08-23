require("class")

Dispatcher = class("Dispatcher")

function Dispatcher:ctor()
    self.events = {}
end

function Dispatcher:add(event, key, callback)
    self.events[event] = self.events[event] or {}
    self.events[event][key] = callback
end

function Dispatcher:remove(event, key)
    self.events[event] = self.events[event] or {}
    self.events[event][key] = nil
end

function Dispatcher:notify(event, ...)
    local events = self.events[event]
    if events then 
        for k,v in pairs(events) do 
            v(event, ...)
        end
    end
end
