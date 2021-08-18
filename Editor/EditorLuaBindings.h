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

	static void bindRender(sol::state& state)
	{
		auto render = state.create_table();
		state["render"] = render;
		auto res_handle = render.new_usertype<ResourceHandle>("ResourceHandle");
		render["create_resource"] = [](int type, int width, int height, int fmt)->ResourceHandle::Ptr {
			auto ret = ResourceHandle::create((Renderer::ViewType)type, width, height,(DXGI_FORMAT) fmt );
			return ret;
		};


		auto pipeline = render.new_usertype<Pipeline>("Pipeline");
		pipeline["execute"] = &Pipeline::execute;
		pipeline["add_pass"] = &Pipeline::addPass;
		pipeline["reset"] = &Pipeline::reset;

		auto renderpass = render.new_usertype<RenderGraph::RenderPass>("RenderPass");
		sol::table opt = state.create_table();
		render["pipeline_operation"] = opt;

	}

};
