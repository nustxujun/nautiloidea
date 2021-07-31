require("class")
require("string_utils")
exists = filesystem.exists
directory_iterator = filesystem.directory_iterator
local search_paths = {"resources"}

local directories = {}
local resource_factories = {}

local Directory = class("Directory")

function Directory:ctor()
	self.sub_dirs = {}
	self.files = {}
end

local Resource = class("Resource")

function Resource:update_file_info()
end

local UnKnownResource = class("UnknownResource", Reosurce)
UnKnownResource.type = "Unknown File"

function UnKnownResource:ctor(path)
	self.path = path:path()
	self.name = path:filename();
	self.ext = path:extension()
end

function create_resource(path)
	local ext = path:extension()
	local fac = resource_factories[ext]
	if (fac) then 
		return fac(path)
	else
		return UnKnownResource(path)
	end
end

function refresh_directory(path, dir)
	for k,v in pairs(directory_iterator(path)) do 
		local name = v:filename()
		local path = v:path()
		if path == "." then 
		elseif v:is_directory() then
			dir.sub_dirs[name] = dir.sub_dirs[name] or Directory()
			refresh_directory(path, dir.sub_dirs[name])
		else
			if dir.files[name] then 
				dir.files[name]:update_file_info()
			else
				dir.files[name] = create_resource(v);	
			end
		end
	end
end

function refresh()
	for k,v in pairs(search_paths) do 
		if exists(v) then
			directories[v] = directories[v] or Directory()
			refresh_directory(v,directories[v])
		end
	end
end

function register_factory(ext, factory)
	resource_factories[ext] = factory
end

return {refresh = refresh, register_factory = register_factory, resource_tree = directories}

