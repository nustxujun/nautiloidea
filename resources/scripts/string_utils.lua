function string.split(str, repl )
	local ret = {}
	local pattern = string.format("([^%s]+)", repl)
    string.gsub(str, pattern, function(w)
        table.insert(ret, w)
	end)
	return ret
end