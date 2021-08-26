#include <sol/sol.hpp>
#include "World.h"
#include "Pipeline.h"
#include "PipelineOperation.h"
class EditorLuaBinding
{
public:
	static void bindWorld(sol::state& state)
	{
		auto world = state.create_table();
		state["world"] = world;
			
		world["create_scene"] = []() {
			return std::make_shared<World>();
		};
	}

	static void bindRender(sol::state& state);

};
