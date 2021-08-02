require("class")
WindowBase = class("WindowBase")

-- WindowBase -------------------------------------------------------
function WindowBase:ctor()
	self.children = {}
	self.remove_list = {}
	self.window_command_list = {}
	self.visible = true
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
	if not self.visible then 
		return 
	end
	if self:begin_window() then 
		self:do_window_command()
		self:update_children()
		self:end_window()
	end
end

function WindowBase:update_children()
	local children = self.children
	for  k,v in pairs(children) do 
		v:update()
	end

	if next(self.remove_list) ~= nil then 
		local cld = {}
		local rmv = self.remove_list
		for k,v in pairs(self.children) do 
			if (not rmv[v]) then 
				cld[#cld + 1] = v
			end
		end
		self.children = cld;
		self.remove_list = {}
	end
end

function WindowBase:begin_window()
	return true
end

function WindowBase:end_window()
end

function WindowBase:set_parent(p)
	self.parent = p
end

function WindowBase:add_child(child)
	assert(child ~= nil)
	self.children[#self.children + 1] = child
	child:set_parent(self)
end

function WindowBase:remove_child(child)
	for k, v in pairs(self.children) do 
		if (child == v) then 
			self.remove_list[v] = true
		end
	end
end

function WindowBase:remove_all_children()
	for k,v in pairs(self.children) do 
		self.remove_list[v] = true
	end
end

function WindowBase:set_pos(x, y)
	self:add_window_command("set_pos",function()
		imgui.SetWindowPos({x,y}, 1)
	end)
end

function WindowBase:set_size(x, y)
	self:add_window_command("set_size",function()
		imgui.SetWindowSize({x,y}, 1)
	end)
end

-- Root -----------------------------------------------------------
Root = WindowBase()
function tick()
	Root:update()
end

-- Window -------------------------------------------------------
Window = class("Window", WindowBase)
function Window:ctor(name, opened, flags)
	self.super(WindowBase).ctor(self)
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

function Window:update()
	if self:begin_window() then 
		self:do_window_command()
		self:update_children()
	end
	self:end_window() -- make end out of the branch of begin
end

-- TabBar ------------------------------------------------------
TabBar = class("TabBar", WindowBase)
function TabBar:ctor(name, flags) 
	self.super(WindowBase).ctor(self)
	self.name = name
	self.flags = flags or 0
end

function TabBar:begin_window()
	return imgui.BeginTabBar(self.name, self.flags)
end

function TabBar:end_window()
	imgui.EndTabBar()
end

-- TabItem ---------------------------------------------------
TabItem = class("TabItem", WindowBase)
function TabItem:ctor(name, opened, flags)
	self.super(WindowBase).ctor(self)
	self.name = name
	self.opened = bool.new()
	self.opened.value = opened or true
	self.flags = flags or 0
end

function TabItem:begin_window()
	return imgui.BeginTabItem(self.name, self.opened.pointer, self.flags)
end

function TabItem:end_window()
	imgui.EndTabItem()
end

-- InputText ---------------------------------------------------

InputText = class("InputText", WindowBase)
function InputText:ctor(name, text, flags, callback)
	self.super(WindowBase).ctor(self)
	self.name = name
	self.text = string_buffer.new()
	self.text.value = text or ""
	self.callback = callback
	self.flags = flags or 0
end

function InputText:begin_window()
	return imgui.InputText(self.name, self.text.pointer, self.flags)
end

-- Text ---------------------------------------------------
Text = class("Text", WindowBase)
function Text:ctor(...)
	self.super(WindowBase).ctor(self)
	self.args = {...}
end

function Text:begin_window()
	return imgui.Text(table.unpack(self.args))
end

-- TreeNode ---------------------------------------------------
TreeNode = class("TreeNode", WindowBase)
function TreeNode:ctor(name)
	self.super(WindowBase).ctor(self)
	self.name = name
end

function TreeNode:begin_window()
	return imgui.TreeNode(self.name)
end

function TreeNode:end_window()
	imgui.TreePop()
end

-- Combo ---------------------------------------------------
Combo = class("Combo", WindowBase)
function Combo:ctor(name, list,selected,flags)
	self.super(WindowBase).ctor(self)
	self.list = list or {}
	self.name = name
	self.selected = selected or 1
	self.flags = flags or 0
end

function Combo:begin_window()
	return imgui.BeginCombo(self.name, self.list[self.selected],self.flags)
end

function Combo:end_window()
	imgui.EndCombo()
end

function Combo:set_selected(index)
	assert(index > 0 and index <= #self.list)
	self.selected = index 
end

-- Selectable ---------------------------------------------------
Selectable = class("Selectable", WindowBase)
function Selectable:ctor(name, selected, click_callback, flags)
	self.super(WindowBase).ctor(self)
	self.name = name
	self.selected = selected or false
	self.click_callback = click_callback 
	self.flags =  flags or 0
end

function Selectable:begin_window()
	local clicked = imgui.Selectable(self.name, self.selected,self.flags)
	if (clicked and self.click_callback) then
		self.click_callback(self)
	end
	return clicked
end

function Selectable:set_selected(val)
	self.selected = val
end

-- Popup ---------------------------------------------------
Popup = class("Popup", WindowBase)
Popup.NORMAL = 0
Popup.POPUP = 1
function Popup:ctor(name, flags)
	self.super(WindowBase).ctor(self)
	self.name = name
	self.flags = flags or 0
	self.state = Popup.NORMAL
end

function Popup:begin_window()
	if self.state == Popup.POPUP then 
		imgui.OpenPopup(self.name, self.flags)
		self.state = Popup.NORMAL
	end
	return imgui.BeginPopup(self.name, self.flags)
end

function Popup:end_window()
	imgui.EndPopup()
end

function Popup:pop()
	self.state = Popup.POPUP
end

-- RETURN --
return _ENV
