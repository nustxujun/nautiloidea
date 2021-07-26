local Gui = require("gui")
local left = Window("left")
Gui.Root:add_child(left)
local width, height = core.get_window_size();
left:set_size(200, height)
left:set_pos(0,0)

local leftTabBar = TabBar("leftTabBar")
left:add_child(leftTabBar)
local resources = TabItem("resources")
local scene = TabItem("scene")

leftTabBar:add_child(resources)
leftTabBar:add_child(scene)

core.ui_update_callback = Gui.tick


local rs = require("resource_system");