
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

leftTabBar:add_child(res_tab)
leftTabBar:add_child(scene_tab)

core.ui_update_callback = Gui.tick


local resource_system = require("resource_system");
local resources = require("resources");

local popup = Popup("popmenu")
Gui.Root:add_child(popup)

function refresh_leftbar()
	local find_input = InputText("find")
	res_tab:add_child(find_input)
	local filter_list = {"all"}
	for k,v in pairs(resource_system.get_type_list()) do 
		filter_list[#filter_list + 1] = k
	end
	local filter = Combo("filter", filter_list)
	res_tab:add_child(filter)
	for k,v in pairs(filter_list) do 
		local index = k;
		local s = Selectable(v, k == 1, function (selectable)
			if last_selected == selectable then 
				return 
			end
			filter.last_selected:set_selected(false)
			filter:set_selected(index)
			selectable:set_selected(true)
			filter.last_selected = selectable
		end)
		filter:add_child(s)
	end
	filter.last_selected = filter.children[1]

	resource_system.refresh()
	local tree = resource_system.resource_tree;

	function build_folder(dir, folder)
		for k,v in pairs(dir.sub_dirs) do 
			local subfolder = TreeNode(k)
			folder:add_child(subfolder)
			build_folder(v,subfolder);
		end

		for k,v in pairs(dir.files) do 
			local res = Selectable(k, false, function (selectable)
				popup:remove_all_children()
				popup:pop()
				popup:add_child(Text(k))
			end)
			folder:add_child(res)
		end
	end

	for k,v in pairs(tree) do 
		local subfolder = TreeNode(k)
		res_tab:add_child(subfolder)
		build_folder(v, subfolder)
	end
end

refresh_leftbar()