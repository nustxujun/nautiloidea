local debugger = require("debugger")
debugger.start()

local Gui = require("gui")
local leftTabBar = Window("left")
Gui.Root:add_child(leftTabBar)

width, height = core.get_window_size();
leftTabBar:set_size(200, height)
leftTabBar:set_pos(0,0)
aa=
-- window_opened = bool.new()
-- window_opened.value = true


-- function ui_update()
-- 	width, height = core.get_window_size();
-- 	imgui.Begin("window", window_opened.pointer, 0)
-- 	imgui.SetWindowSize({200,height}, 1)
-- 	imgui.SetWindowPos({0,0}, 0)
-- 	if (imgui.BeginTabBar("leftTabBar", 0)) then 
-- 		if (imgui.BeginTabItem("resource")) then
-- 			imgui.EndTabItem()
-- 		end
-- 		if (imgui.BeginTabItem("scene")) then 
-- 			imgui.EndTabItem()
-- 		end

-- 		imgui.EndTabBar()
-- 	end

-- 	imgui.End()

-- 	print ("ui_end")
-- end


core.ui_update_callback = Gui.tick