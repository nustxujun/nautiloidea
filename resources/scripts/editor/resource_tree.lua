
local Gui = require("gui")
local resource_popup = require("editor/resource_popup")
local resource_tree = Window("resources", true)
Gui.Root:add_child(resource_tree)


local resource_system = require("resource/resource_system");
local resources = require("resource/resources");

local popup = Popup("popmenu")
Gui.Root:add_child(popup)

function refresh_leftbar()
	resource_tree:remove_all_children()

	local find_input = InputText("find")
	resource_tree:add_child(find_input)
	local filter_list = {"all"}
	for k,v in pairs(resource_system.get_type_list()) do 
		filter_list[#filter_list + 1] = k
	end
	local filter = Combo("filter", filter_list)
	resource_tree:add_child(filter)
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
				resource_popup.popup(v, popup)
				popup:pop()
			end)
			folder:add_child(res)
		end
	end

	for k,v in pairs(tree) do 
		local subfolder = TreeNode(k)
		resource_tree:add_child(subfolder)
		build_folder(v, subfolder)
	end
end

refresh_leftbar()


return {
	refresh = refresh_leftbar,
	show = function () resource_tree.opened.value = true end
}