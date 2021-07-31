local Gui = require("gui")
local left = Window("left")
Gui.Root:add_child(left)
local width, height = core.get_window_size();
left:set_size(200, height)
left:set_pos(0,0)

local leftTabBar = TabBar("leftTabBar")
left:add_child(leftTabBar)
local res_tab = TabItem("resources")
local scene_tab = TabItem("scene")
local find_input = InputText("find")

-- leftTabBar:add_child(find_input)
leftTabBar:add_child(res_tab)
leftTabBar:add_child(scene_tab)

core.ui_update_callback = Gui.tick


local resource_system = require("resource_system");
local resources = require("resources");


function refresh_leftbar()
	resource_system.refresh()
	local tree = resource_system.resource_tree;

	function build_folder(dir, folder)
		for k,v in pairs(dir.sub_dirs) do 
			local subfolder = TreeNode(k)
			folder:add_child(subfolder)
			print (k, "c f")
			-- build_folder(v,subfolder);
		end

		-- for k,v in pairs(dir.files) do 
		-- 	local res = Text(k)
		-- 	folder:add_child(res)
		-- end
	end

	for k,v in pairs(tree) do 
		local subfolder = TreeNode(k)
		res_tab:add_child(subfolder)
		build_folder(v, subfolder)
	end
end

refresh_leftbar()