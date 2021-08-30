local Gui = require("gui")
local Viewport = require("editor/viewport")
local preview_window = Viewport("preview")
require("pass")

preview_window.window.opened.value = false

function popup_ModelResource(res, popup)
	popup:add_child(MenuItem("preview","", false, true, function ()
		preview_window:show()
		local win = preview_window.window
		local root = world.load_static_mesh_from_file(res.path)
		local rt = PassResource()
		local ds = PassResource()
		win:add_property_listener("get_window_size", rt, function()
			local w = math.floor(win.width)
			local h = math.floor(win.height)
			rt:reset(1,w ,h ,10,{1.0,0.0,0.0,1.0})
			ds:reset(2,w ,h ,45)

			preview_window.window:remove_all_children()
			preview_window.window:add_child(Image(rt, w,h))

		end)
		local pass = ScenePass("model preview")
		pass:bind_resource("rt", rt)
		pass:bind_resource("ds", ds)
		pass:set_root(root)
		local pp = preview_window.pipeline
		pp:reset()
		pp:push_back(pass)
	end))
end


return {
	popup = function (res, popup)
		local func = _ENV["popup_" .. res.__class_type]
		if not func then 
			return 
		end

		func(res, popup)
	end
}
