local Gui = require("gui")
local Viewport = require("editor/viewport")
local preview_window = Viewport("preview")
require("pass")

preview_window.window.opened.value = false

function popup_ModelResource(res, popup)
	popup:add_child(MenuItem("preview","", false, true, function ()
		preview_window:show()
		-- local win = preview_window.window
		-- local root = world.load_static_mesh_from_file(res.path)
		-- local rt = PassResource(function(rt)
		-- 	local w,h = win:get_size()
		-- 	rt:reset(1,w,h,28)
		-- end)
		-- local ds = PassResource(function(rt)
		-- 	local w,h = win:get_size()
		-- 	rt:reset(1,w,h,28)
		-- end)
		-- local pass = ScenePass()
		-- pass:bind_resource("rt", rt)
		-- pass:bind_resource("ds", ds)
		-- pass:set_root(root)
		-- local pp = preview_window.pipeline
		-- pp:reset()
		-- pp:push_back(pass)
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
