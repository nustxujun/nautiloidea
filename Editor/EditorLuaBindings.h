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

		// ResourceHandle
		auto res_handle = render.new_usertype<ResourceHandle>("ResourceHandle");
		res_handle["get_gpu_descriptor_handle"] = [](ResourceHandle::Ptr rs) {
			return (void*)rs->getView()->getShaderResource().ptr;
		};
		render["create_resource"] = [](int type, int width, int height, int fmt)->ResourceHandle::Ptr {
			auto ret = ResourceHandle::create((Renderer::ViewType)type, width, height,(DXGI_FORMAT) fmt );
			return ret;
		};

		// pipeline
		auto pipeline = render.new_usertype<Pipeline>("Pipeline");
		pipeline["execute"] = &Pipeline::execute;
		pipeline["add_pass"] = &Pipeline::addPass;
		pipeline["reset"] = &Pipeline::reset;

		// renderpass
		auto renderpass = render.new_usertype<RenderGraph::RenderPass>("RenderPass");
		sol::table opt = state.create_table();
		render["pipeline_operation"] = opt;

		// StaticTexture
		auto static_tex = render.new_usertype<Renderer::Resource::Ref>("StaticTexture");
		static_tex["get_gpu_descriptor_handle"] = [](Renderer::Resource::Ref res){
			return (void*)res->getShaderResource().ptr;
		};
		static_tex["get_width"] = [](Renderer::Resource::Ref res) {
			return res->getDesc().Width;
		};
		static_tex["get_height"] = [](Renderer::Resource::Ref res) {
			return res->getDesc().Height;
		};

		render["create_texture_from_file"] = [](std::string filename, bool srgb) {
			return Renderer::getSingleton()->createTextureFromFile(std::move(filename), srgb);
		};
	}

};
