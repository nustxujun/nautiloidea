require("class")
local Gui = require("gui")

local Viewport = class("Viewport")

function Viewport:ctor(name)
	self.window = Window(name, true)
	Gui.Root:add_child(self.window)
end

function Viewport:show()
	self.window.opened.value = true
end

return Viewport