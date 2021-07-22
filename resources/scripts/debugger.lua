require("string_utils")
traceback = debug.traceback
getinfo = debug.getinfo
getlocal = debug.getlocal
breakers = {}

CONTINUE = 0
STEP_INTO = 1
STEP_OVER = 2
STEP_OUT = 3

step_state = STEP_INTO
step_target = nil
step_line = 0
current_frame = {}

function add_breaker(source, lineno)
	breakers[source] = breakers[source] or {}
	local tbl = breakers[source]
	tbl[#tbl + 1] = tonumber(lineno)
	print("add breaker on ", source, lineno)
end

function format_obj(obj)
	if (type(obj) == "table")then
		local cont = "";
		for k,v in pairs(obj) do 
			cont = cont .. " " .. tostring(k) .. " = " .. format_obj(v) .. ","
		end
		return "{ " .. cont .. " }"
	else
		return tostring(obj)
	end
end

function print_any(obj)
	print(format_obj(obj))
end

function parse_cmd(cmd, info)
	cmdlist = string.split(cmd, " ")
	cmd = cmdlist[1]
	if (cmd == "t") then 
		print (traceback(0))
	elseif (cmd == "c") then 
		return false
	elseif (cmd == "s") then 
		step_state = STEP_INTO
		return false
	elseif cmd == "so" then
		step_state = STEP_OVER
		step_target = current_frame.func
		return false
	elseif cmd == "sr" then 
		step_state = STEP_OUT
		step_target = current_frame.func
		return false
	elseif (cmd == "i") then 
		for k,v in pairs(info) do 
			print (k,v )
		end
	elseif (cmd == "b") then
		if cmdlist[2] then 
			line = cmdlist[3] or 1
			add_breaker(cmdlist[2],line)
		else
			b,e = string.find(info.short_src,"%a+%.lua")
			add_breaker(string.sub(info.short_src, b,e),info.currentline)
		end
	elseif cmd == "l" and cmdlist[2] ~= nil then
		local i = 1
		while true do 
			local k,v = getlocal(cmdlist[2], i)
			i = i + 1
			if not k or not v then 
				break;
			end
			print (k,format_obj(v))
		end
	elseif debug[cmd] then
		print_any(debug[cmd](cmdlist[2], cmdlist[3],cmdlist[4]))
	end

	return true
end

function find_breaker(info)

	local src = info.short_src
	b,e = string.find(src,"%a+%.lua")
	if b == nil or e == nil then 
		return false
	end
	list =  breakers[string.sub(src, b,e)]
	if (list) then 
		lineno = tonumber(info.currentline)
		for k,v in pairs(list) do 
			if v == lineno then
				print("find breaker on ",string.sub(src, b,e), lineno )
				return true
			end
		end
	end
	return false
end

abs = math.abs
function show_code(info)

	short_src = info.short_src
	local file = io.open(short_src, r);
	if not file then 
		print (short_src, info.currentline)
		return
	end

	local index = 1
	local lineno = info.currentline
	for line in file:lines() do 
		if (abs(index - lineno) <= 3) then 
			local prefix = " "
			if (index == lineno) then 
				prefix = ">"
			end
			print(short_src, index,prefix, line)
		end
		index = index + 1
	end
	file:close()
end

function do_break(info)

	show_code(info)

	step_state = CONTINUE
	local b = true
	while b do
		-- print (info.short_src, info.currentline)
		cmd = core.input()
		b = parse_cmd(cmd, info) 
	end

end

function check_step(event)
	if step_state == STEP_INTO and event == "line"  then 
		return true
	elseif step_state == STEP_OVER and event == "line" and step_target == current_frame.func then 
		return true
	elseif step_state == STEP_OUT and event == "return" and step_target == current_frame.func then 
		return true
	end
	return false
end

function hook(event, line)
	info = getinfo(2)
	current_frame = info
	if (check_step(event) or find_breaker(current_frame)) then
		do_break(info);
	end
end

function start()
	-- current_frame = getinfo(1)
	-- do_break(current_frame);
	debug.sethook(hook, "crl")
end

start()
return { do_break = do_break, break_here = do_break}