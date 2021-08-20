
local Gui = require("gui")

local Root = Gui.Root
require("dispatcher")
local dispacher = Dispatcher()

local editor_main = Window("editor_main",true, 0x00282427)
Root:add_child(editor_main)
-- editor_main:set_size(core.get_window_size())
editor_main:add_window_command("set_size", function ()
	imgui.SetWindowSize({core.get_window_size()}, 1)
end)
editor_main:set_pos(0,0)
editor_main:add_style_var(1, 0,0)


-- mainmenu-------------------------------

local resource_tree = require("editor/resource_tree")


local main_menu = MenuBar()
editor_main:add_child(main_menu)
local view = Menu("view")
view:add_child(MenuItem("resources", "", false, true, function()
	resource_tree.refresh()
	resource_tree.show()
end))

main_menu:add_child(view)

local Viewport = require("editor/viewport")
local scene_vp = Viewport("scene")

local scene = Menu("scene")
view:add_child(MenuItem("scene", "", false, true, function()
	scene_vp:show()
end))