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

	static constexpr int MAX_STRING_SIZE = 256;
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


		registerBaseTypeObject<Wrapper<int>>(lua, "int");
		registerBaseTypeObject<Wrapper<bool>>(lua, "bool");

		struct StringBuffer
		{
			char value[MAX_STRING_SIZE];
			void* pointer = value ;

		};
		auto type = lua.new_usertype<StringBuffer>("string_buffer");
		type["value"] = sol::property(
			[](StringBuffer& sb){return sb.value;}, 
			[](StringBuffer& sb, std::string str){
				ASSERT(str.size() <= MAX_STRING_SIZE, "string buffer is out of range");
				memcpy(sb.value, str.data(), str.size() + 1);
			});
		type["pointer"] = &StringBuffer::pointer;


		registerFunctions(module);
		return module;
	}

	template<class T>
	static void registerBaseTypeObject(sol::state_view& s, const char* name)
	{
		auto type = s.new_usertype<T>(name);
		type["value"] = &T::value;
		type["pointer"] = &T::pointer;
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
		REGISTER_ORIGIN(Text);
		REGISTER_ORIGIN(SameLine);
		REGISTER_ORIGIN(TreePop);



		module["TreeNode"] = [](std::string name){
			return ImGui::TreeNode(name.c_str());
		};

		module["SetWindowSize"]= +[](sol::table size, int flag)
		{
			ImGui::SetWindowSize({ size[1],size[2]}, flag);
		};
		module["SetWindowPos"] = +[](sol::table pos, int flag)
		{
			ImGui::SetWindowPos({ pos[1],pos[2] }, flag);
		};

		module["InputText"] = +[](std::string name, void* str_ptr, int flag){
			return ImGui::InputText(name.c_str(), (char*)str_ptr, MAX_STRING_SIZE, flag);
		};
	}

};