require("class")
exist = filesystem.exist
directory_iterator = filesystem.directory_iterator
local search_paths = {}

local resources = {}

local Directory = class("Directory")

function refresh_directory(path, dir)
	for k,v in pairs(directory_iterator()) do 
		local path = v:path()
		if path == "." then 
		elseif v:is_directory() then
			dir[path] = dir[path] or {}
			refresh_directory(path, dir[path])
		else
			
		end
	end
end

function refresh()
	for k,v in pairs(search_paths) do 
		if exist(v) then
			resources[v] = resources[v] or {}
			refresh_directory(v,resource[v])
		end
	end
end

return {}

