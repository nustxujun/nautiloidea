#pragma once 

#include <sol/sol.hpp>
#include "imgui/imgui.h"


class ImGuiLuaBinding
{
	template<class T>
	struct Wrapper
	{
		T value;
		T* pointer = &value;
	};
public:
	static void bind(sol::state& state)
	{
		state.require("imgui",
			sol::c_call<decltype(&ImGuiLuaBinding::open), &ImGuiLuaBinding::open>);
	}
private:
	static sol::table open(sol::this_state s)
	{
		sol::state_view lua(s);
		
		sol::table module = lua.create_table();


		registerBaseTypeObject<int>(lua, "int");
		registerBaseTypeObject<bool>(lua, "bool");

		registerFunctions(module);
		return module;
	}

	template<class T>
	static void registerBaseTypeObject(sol::state_view& s, const char* name)
	{
		using Wrapper = Wrapper<T>;
		auto type = s.new_usertype<Wrapper>(name);
		type["value"] = &Wrapper::value;
		type["pointer"] = &Wrapper::pointer;
	}

	static void registerFunctions(sol::table& module)
	{

#define REGISTER_ORIGIN(x) module[#x] = &ImGui::x;
		REGISTER_ORIGIN(Begin);
		REGISTER_ORIGIN(End);
		REGISTER_ORIGIN(BeginTabBar);
		REGISTER_ORIGIN(EndTabBar);
		REGISTER_ORIGIN(BeginTabItem);
		REGISTER_ORIGIN(EndTabItem);
		

		module["SetWindowSize"]= +[](sol::table size, int flag)
		{
			ImGui::SetWindowSize({ size[1],size[2]}, flag);
		};
		module["SetWindowPos"] = +[](sol::table pos, int flag)
		{
			ImGui::SetWindowSize({ pos[1],pos[2] }, flag);
		};


	}

};