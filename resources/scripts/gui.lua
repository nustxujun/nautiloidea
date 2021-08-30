require("class")



local PropertyMonitor = class("PropertyMonitor")
local property_getter = 
{
	get_window_size = imgui.GetWindowSize,
	get_window_pos = imgui.GetWindowPos,
}

function PropertyMonitor:ctor()
	self.listeners = {}
end


function PropertyMonitor:add_listener(prop, key, func)
	function tbl()
		local t = {}
		setmetatable(t, {__mode = "k"});
		return t
	end
	self.listeners[prop] = self.listeners[prop] or tbl()
	self.listeners[prop][key] = func
end

function PropertyMonitor:update(win)
	for prop,cont in pairs(self.listeners) do 
		local ret = property_getter[prop]()
		local notify = false
		for k,v in pairs(ret) do 
			if win[k] ~= v then 
				win[k] = v
				notify = true
			end
		end
		if notify then 
			for k,v in pairs(cont) do 
				v(ret)
			end
		end
	end
end

-- WindowBase -------------------------------------------------------
WindowBase = class("WindowBase")
function WindowBase:ctor()
	self.children = {}
	self.remove_list = {}
	self.window_command_list = {}
	self.visible = true
	self.style_var_list = {}
	self.is_drawn = false
	self.property_monitor = PropertyMonitor()
end

function WindowBase:add_window_command(name, cmd)
	self.window_command_list[name] = cmd
end

function WindowBase:do_window_command()
	for k,v in pairs(self.window_command_list) do 
		v()
	end
	self.property_monitor:update(self)
end

function WindowBase:push_style_var()
	for k,v in pairs(self.style_var_list) do 
		imgui.PushStyleVar(table.unpack(v))
	end
end

function WindowBase:pop_style_var()
	local count = #self.style_var_list
	if count ~= 0 then 
		imgui.PopStyleVar(#self.style_var_list)
	end
end

function WindowBase:add_style_var(id, ...)
	for k,v in pairs(self.style_var_list) do 
		if (v[0] == id) then 
			self.style_var_list[k] = {id, ...}
			return
		end
	end
	self.style_var_list[#self.style_var_list + 1] = {id, ...}
end

function WindowBase:remove_style_var(id)
	for k,v in pairs(self.style_var_list) do 
		if (v[0] == id) then 
			table.remove(self.style_var_list, k)
		end
	end
end

function WindowBase:update()
	if not self.visible then 
		return 
	end
	self:push_style_var()
	self.is_drawn = self:begin_window()
	if self.is_drawn then 
		self:pop_style_var()
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
	assert(child.parent == nil)
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

function WindowBase:add_property_listener(prop, key, lis)
	self.property_monitor:add_listener(prop, key, lis)
end
-- Root -----------------------------------------------------------
Root = WindowBase()
function tick()
	-- imgui.ShowDemoWindow()
	Root:update()
end

-- Operator -------------------------------------------------------
Operator = class("Operator", WindowBase)
function Operator:ctor()
	self.visible = true
end
function Operator:update()
	if not self.visible then 
		return 
	end
	self.is_drawn = self:begin_window()
end 

-- Widget -------------------------------------------------------
LightWidget = class("LightWidget", WindowBase)
function LightWidget:ctor(...)
	WindowBase.ctor(self, ...)
end

function LightWidget:update()
	if not self.visible then 
		return 
	end
	self:push_style_var()
	self.is_drawn = self:begin_window()
	self:pop_style_var()
end



-- Window -------------------------------------------------------
Window = class("Window", WindowBase)
function Window:ctor(name, opened, flags)
	self.super(WindowBase).ctor(self)
	self.name = name
	self.opened = bool.new()
	self.opened.value = opened == nil and true or opened
	self.flags = flags or 0
end

function Window:begin_window()
	return imgui.Begin(self.name, self.opened.pointer, self.flags)
end

function Window:end_window()
	imgui.End()
end

function Window:dock_space()
	imgui.DockSpace(self.name, 0,0, 0)
end

function Window:add_dock_node()
	if (self.flags &  0x200000) ~= 0 then 
		self:dock_space()
	end
end

function Window:update()
	if not self.visible or not self.opened.value then 
		return 
	end
	self:push_style_var()
	self.is_drawn = self:begin_window()
	if self.is_drawn then 
		self:pop_style_var()
		self:add_dock_node()
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
	self.opened.value = opened == nil and true or opened
	self.flags = flags or 0
end

function TabItem:begin_window()
	return imgui.BeginTabItem(self.name, self.opened.pointer, self.flags)
end

function TabItem:end_window()
	imgui.EndTabItem()
end

-- InputText ---------------------------------------------------

InputText = class("InputText", LightWidget)
function InputText:ctor(name, text, flags, callback)
	self.super(LightWidget).ctor(self)
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
Text = class("Text", LightWidget)
function Text:ctor(...)
	self.super(LightWidget).ctor(self)
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
Selectable = class("Selectable", LightWidget)
function Selectable:ctor(name, selected, click_callback, flags)
	self.super(LightWidget).ctor(self)
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

-- MainMenuBar ---------------------------------------------------
MainMenuBar = class("MainMenuBar", WindowBase)
function MainMenuBar:ctor()
	self.super(WindowBase).ctor(self)
end

function MainMenuBar:begin_window()
	return imgui.BeginMainMenuBar()
end

function MainMenuBar:end_window()
	imgui.EndMainMenuBar()
end

-- MenuBar ---------------------------------------------------
MenuBar = class("MenuBar", WindowBase)
function MenuBar:ctor()
	self.super(WindowBase).ctor(self)
end

function MenuBar:begin_window()
	return imgui.BeginMenuBar()
end

function MenuBar:end_window()
	imgui.EndMenuBar()
end

-- Menu ---------------------------------------------------
Menu = class("Menu", WindowBase)
function Menu:ctor(name, enable)
	self.super(WindowBase).ctor(self)
	self.name = name
	self.enable = enable == nil and true or enable
end

function Menu:begin_window()
	return imgui.BeginMenu(self.name, self.enable)
end

function Menu:end_window()
	imgui.EndMenu()
end

-- ChildWindow ---------------------------------------------------
ChildWindow = class("ChildWindow", WindowBase)
function ChildWindow:ctor(name, width, height, border, flags)
	self.super(WindowBase).ctor(self)
	self.name = name
	self.width = width
	self.height = height
	self.border = border == nil and true or border
	self.flags = flags or 0
end

function ChildWindow:begin_window()
	return imgui.BeginChild(self.name,self.width, self.height, self.border, self.flags)
end

function ChildWindow:end_window()
	imgui.EndChild()
end

function ChildWindow:update()
	self.is_drawn = self:begin_window()
	if self.is_drawn then 
		self:do_window_command()
		self:update_children()
	end
	self:end_window() -- make end out of the branch of begin
end

-- MenuItem ---------------------------------------------------
MenuItem = class("MenuItem", LightWidget)
function MenuItem:ctor(name, short_cut, selected, enable, callback)
	self.super(LightWidget).ctor(self)
	self.name = name
	self.short_cut= short_cut or ""
	self.selected = selected or false
	self.enable = enable == nil and true or enable
	self.callback = callback or function () end
end

function MenuItem:begin_window()
	local ret = imgui.MenuItem(self.name, self.short_cut, self.selected, self.enable)
	if ret then 
		self.callback(self)
	end
	return ret 
end

function MenuItem:set_callback(cb)
	self.callback = cb or function() end
end

-- Image ---------------------------------------------------
Image = class("Image", LightWidget)
function Image:ctor(tex_handle, width, height,uv_range)
	self.super(LightWidget).ctor(self)
	self.texture = tex_handle
	self.width = width 
	self.height = height
	self.uv_range = uv_range or {0,0, 1,1}
end

function Image:begin_window()
	return imgui.Image(self.texture:get_gpu_descriptor_handle(), self.width, self.height, self.uv_range)
end

-- Columns ---------------------------------------------------
Columns = class("Columns", Operator)
function Columns:ctor(count, border)
	self.super(Operator).ctor(self)
	self.count = count or 1
	self.border = border == nil and true or border
	self.id = tostring(self)
end

function Columns:begin_window()
	imgui.Columns(self.count, self.id, self.border)
end

-- NewLine ---------------------------------------------------
NewLine = class("NewLine", Operator)

function NewLine:begin_window()
	imgui.NewLine()
end

-- Spacing ---------------------------------------------------
Spacing = class("Spacing", Operator)

function Spacing:begin_window()
	imgui.Spacing()
end

-- RETURN --
return _ENV
