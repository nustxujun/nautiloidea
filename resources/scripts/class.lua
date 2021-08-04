


function class(classname, ...)
	function generate_super(...)
		local supers = {...}
		local super_table = {}
		local super_list = {}
	
		for _, s in pairs(supers) do 
			local obj = {}
			setmetatable(obj, {__index = s})
			super_table[s.__class_type] = obj
			super_list[#super_list + 1] = obj
		end
	
		local super = { __super_list = super_list, __super_table = super_table}
	
		function get(self, key)
			for _, s in pairs(super_list) do 
				if (s[key] ~= nil) then 
					return s[key]
				end
			end
			return nil
		end
		function get_super( self, class_type)
			if (class_type == nil) then
				return super_list[1]
			elseif (type(clas_type) == "string") then
				return super_table[class_type]		
			else
				return super_table[class_type.__class_type]		
			end
		end
		setmetatable(super, {__index = get, __call = get_super})
		return super
	end
	
	function new(class_type, ...)
		local obj = {}
		setmetatable(obj, { __index = class_type })
		obj:ctor(...)
		return obj
	end

	local super = generate_super(...)
	local obj = {
		__class_type = classname,
		super = super,
		new = new,
		ctor = super() == nil and function(...) end or super().ctor
	}
	function get(self, key)
		local val = rawget(self, key)
		if (val == nil) then 
			return super[key]
		else
			return val
		end
	end

	setmetatable(obj, {__index = get, __call = new})
	return obj
end