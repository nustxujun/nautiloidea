require("class")

Dispatcher = class("Dispatcher")

function Dispatcher:ctor()
    self.events = {}
end

function Dispatcher:add(event, callback)
    self.events[event] = self.events[event] or {}
    self.events[event][callback] = true
end

function Dispatcher:remove(event, callback)
    self.events[event] = self.events[event] or {}
    self.events[event][callback] = nil
end

function Dispatcher:notify(event, ...)
    local events = self.events[event]
    if events then 
        for k,v in pairs(events) do 
            k(event, ...)
        end
    end
end
