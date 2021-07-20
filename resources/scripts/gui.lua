require("class")

WindowBase = class("WindowBase")
WindowBase.visible = true

-- WindowBase -------------------------------------------------------
function WindowBase:ctor()
	self.children = {}
	self.remove_list = {}
	self.window_command_list = {}
end

function WindowBase:add_window_command(name, cmd)
	self.window_command_list[name] = cmd
end

function WindowBase:do_window_command()
	for k,v in pairs(self.window_command_list) do 
		v()
	end
end

function WindowBase:update()
	if (self.visible) then
		if self:begin_window() then 
			self:do_window_command()
			self:update_children()
			self:end_window()
		end
	end
end

function WindowBase:update_children()
	children = self.children
	for  k,v in pairs(children) do 
		v:update()
	end

	for i in ipairs(self.remove_list) do 
		table.remove(children, i)
	end
	self.remove_list = {}
end

function WindowBase:begin_window()
	return true
end

function WindowBase:end_window()
end

function WindowBase:add_child(child)
	assert(child ~= nil)
	self.children[#self.children + 1] = child
end

function WindowBase:remove_child(child)
	for k, i in pairs(self.children) do 
		if (child == i) then 
			self.remove_list[#self.remove_list + 1] = k
		end
	end
end

function WindowBase:set_pos(x, y)
	self:add_window_command("set_pos",function()
		imgui.SetWindowPos({x,y}, 1)
	end)
end

function WindowBase:set_size(x, y)
	self:add_window_command("set_size",function()
		print ("window size", x,y)
		imgui.SetWindowSize({x,y}, 1)
	end)
end

-- Window -------------------------------------------------------
Window = class("Window", WindowBase)
function Window:ctor(name, opened, flags)
	self.super(WindowBase):ctor()
	self.name = name
	self.opened = bool.new()
	self.opened.value = opened or true
	self.flags = flags or 0
end

function Window:begin_window()
	return imgui.Begin(self.name, self.opened.pointer, self.flags)
end

function Window:end_window()
	imgui.End()
end



-- Root -----------------------------------------------------------
Root = WindowBase()
function tick()
	Root:update()
end
return _ENV