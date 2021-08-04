local Gui = require("gui")
local Viewport = require("editor/viewport")
local preview_window = Viewport("preview")
preview_window.window.opened.value = false

return {
	popup = function (res, popup)
		if "ModelResource" == res.__class_type then 
			popup:add_child(MenuItem("preview","", false, true, function ()
				preview_window:show()
			end))
		end
	end
}
