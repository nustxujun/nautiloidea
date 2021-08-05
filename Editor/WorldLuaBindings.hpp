#include <sol/sol.hpp>
#include "World.h"

class WorldLuaBinding
{
public:
	static void bind(sol::state& state)
	{
		auto world = state.create_table();
		state["world"] = world;
			
		world["create_scene"] = []() {
			return std::make_shared<World>();
		};
	}


};
