#pragma once 

#include <sol/sol.hpp>
#include "imgui/imgui.h"

#define IMGUI_DOCKING 1


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


		registerFunctions(module, lua);
		return module;
	}

	template<class T>
	static void registerBaseTypeObject(sol::state_view& s, const char* name)
	{
		auto type = s.new_usertype<T>(name);
		type["value"] = &T::value;
		type["pointer"] = &T::pointer;
	}

	static void registerFunctions(sol::table& module, sol::state_view lua)
	{
		module["ShowDemoWindow"] = []() {
			ImGui::ShowDemoWindow();
		};
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
		REGISTER_ORIGIN(TreePop);
		REGISTER_ORIGIN(BeginCombo);
		REGISTER_ORIGIN(EndCombo);
		//REGISTER_ORIGIN(OpenPopup);
		REGISTER_ORIGIN(BeginPopup);
		REGISTER_ORIGIN(EndPopup);
		REGISTER_ORIGIN(BeginMainMenuBar);
		REGISTER_ORIGIN(EndMainMenuBar);
		REGISTER_ORIGIN(BeginMenu);
		REGISTER_ORIGIN(EndMenu);
		REGISTER_ORIGIN(EndChild);
		REGISTER_ORIGIN(BeginMenuBar);
		REGISTER_ORIGIN(EndMenuBar);
		REGISTER_ORIGIN(PopStyleVar);
		


		REGISTER_ORIGIN(Columns);
		REGISTER_ORIGIN(SameLine);
		REGISTER_ORIGIN(NewLine);
		REGISTER_ORIGIN(Spacing);

		module["Image"] = +[](void* gpu_handle, float width, float height, sol::table uv_range) {
			ImVec2 uv_min = {0,0};
			ImVec2 uv_max = { 1,1 };
			if (uv_range.valid() && uv_range.size() == 4)
			{
				uv_min = { float(uv_range[1]), float(uv_range[2]) };
				uv_max = { float(uv_range[3]), float(uv_range[4]) };
			}
			return ImGui::Image(gpu_handle, { width, height }, uv_min, uv_max);
		};
		
		module["MenuItem"] = (bool(*)(const char* , const char* , bool , bool ))&ImGui::MenuItem;

		module["PushStyleVar"] = sol::overload((void(*)(ImGuiStyleVar, float))&ImGui::PushStyleVar, [](int s, float w, float h){ImGui::PushStyleVar(s,{w,h});});

		module["OpenPopup"] = [](std::string id, int flags) {
			return ImGui::OpenPopup(id.c_str(), flags);
		};
		module["BeginChild"] = [](std::string name, float w, float h, bool border, int flags)
		{
			return ImGui::BeginChild(name.c_str(), { w,h }, border, flags);
		};

		module["Selectable"] = [](std::string name ,bool selected ,int flags){
			return ImGui::Selectable(name.c_str(), selected,flags);
		};
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

		module["GetWindowSize"] = [=]()mutable{
			auto size = ImGui::GetWindowSize();
			sol::table ret = lua.create_table();
			ret["width"] = size.x;
			ret["height"] = size.y;
			return ret;
		};


#if IMGUI_DOCKING
		module["DockSpace"] = [](std::string id, float w, float h, int flags) {
			return ImGui::DockSpace(ImGui::GetID(id.c_str()), { w,h }, flags);
		};
#endif
	}

};