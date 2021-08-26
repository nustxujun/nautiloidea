#include "EditorLuaBindings.h"
#include "StaticMesh.h"


void EditorLuaBinding::bindWorld(sol::state& state)
{
	auto world = state.create_table();
	state["world"] = world;

	world["create_scene"] = []() {
		return std::make_shared<World>();
	};

	auto node = world.new_usertype<Node::Ptr>("Node");

	world["load_static_mesh_from_file"] = &StaticMeshLoader::operator();


}

void EditorLuaBinding::bindRender(sol::state& state)
{
	auto render = state.create_table();
	state["render"] = render;

	// ResourceHandle
	auto res_handle = render.new_usertype<ResourceHandle>("ResourceHandle");
	res_handle["get_gpu_descriptor_handle"] = [](ResourceHandle::Ptr rs) {
		return (void*)rs->getView()->getShaderResource().ptr;
	};
	render["create_resource"] = [](int type, int width, int height, int fmt)->ResourceHandle::Ptr {
		auto ret = ResourceHandle::create((Renderer::ViewType)type, width, height, (DXGI_FORMAT)fmt);
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
	static_tex["get_gpu_descriptor_handle"] = [](Renderer::Resource::Ref res) {
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


	auto opt = render["pipeline_operation"];
	//opt["render_scene"] = [this](ResourceHandle::Ptr rt, ResourceHandle::Ptr ds)->RenderGraph::RenderPass {
	//	return PipelineOperation::renderScene(
	//		{},
	//		std::bind(&Editor::renderScene, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4),
	//		rt, ds);
	//};

	opt["render_scene"] = [](Node::Ptr root, ResourceHandle::Ptr rt, ResourceHandle::Ptr ds) {
		std::vector<RenderObject::Ptr> ros;
		root->visitObject<RenderObject>([&](RenderObject::Ptr ro) {
			ros.push_back(ro);
		});

		PipelineOperation::renderScene({}, [ros = std::move(ros)](Renderer::CommandList* cmdlist, const Pipeline::CameraInfo& cam, UINT flags, UINT mask) {
			for (auto& ro : ros)
				ro->draw(cmdlist);
		}, rt, ds);
	};

	opt["render_ui"] = &PipelineOperation::renderUI;

	opt["present"] = &PipelineOperation::present;
}
