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
		root:update()
		local rt = PassResource()
		local ds = PassResource()
		local camera = world.create_camera();
		win:add_property_listener("get_window_size", rt, function()
			local w = math.floor(win.width)
			local h = math.floor(win.height)
			rt:reset(1,w ,h ,10,{0.0,0.0,0.0,1.0})
			ds:reset(2,w ,h ,45, {1.0, 0})

			preview_window.window:remove_all_children()
			preview_window.window:add_child(Image(rt, w,h))
			camera:set_viewport(0,0,win.width,win.height, 0.0,1.0)
			camera:set_scissor(0,0,w,h)
			camera:set_projection(0.75, win.width / win.height, 0.1, 100)
			camera:set_view(math3d.vec3.new(0,0,10),math3d.vec3.new(0,0,0), math3d.vec3.new(0,1,0))
		end)
		local pass = ScenePass("model preview", root,camera)
		pass:bind_resource("rt", rt)
		pass:bind_resource("ds", ds)
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
