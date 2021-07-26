#pragma once 

#include <sol/sol.hpp>
#include <filesystem>
class LuaFileSystem
{
public:
	static void bind(sol::state& state)
	{
		state.require("filesystem",
			sol::c_call<decltype(&LuaFileSystem::open), &LuaFileSystem::open>);
	}
private:
	static sol::table open(sol::this_state s)
	{
		sol::state_view lua(s);

		sol::table module = lua.create_table();


		registerFunctions(module, lua);
		return module;
	}

	static void registerFunctions(sol::table& module, sol::state_view s)
	{
		namespace fs = std::filesystem;
		auto directory_entry = s.new_usertype<fs::directory_entry>("directory_entry");
		directory_entry["is_directory"] = [](fs::directory_entry& de){
			return de.is_directory();
		};
		directory_entry["path"] = [](fs::directory_entry& de){
			return std::string(de.path().filename().string());
		};

		module["exists"] = [](std::string path){
			return fs::exists(path);
		};
		module["absolute"] = [](std::string path){
			return fs::absolute(path).string();
		};
		module["directory_iterator"] = [s = s](std::string path)mutable{
			sol::table paths = s.create_table();
			for (auto& f : fs::directory_iterator(path))
			{
				paths.add(f);
			}
			return paths;
		};


	}
};