#include <sol/sol.hpp>
#include "World.h"
#include "Pipeline.h"
#include "PipelineOperation.h"
class EditorLuaBinding
{
public:
	static void bindWorld(sol::state& state);

	static void bindRender(sol::state& state);

};
