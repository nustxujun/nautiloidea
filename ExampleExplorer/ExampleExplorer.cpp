#include "ExampleExplorer.h"
#include "ExampleFramework.h"
#include "Framework.h"
#include "Profile.h"
void ExampleExplorer::init()
{
	char tmp[256] = {};
	::GetCurrentDirectoryA(256,tmp);
	Renderer::getSingleton()->addSearchPath("engine/");
	Renderer::getSingleton()->addSearchPath("engine/Shaders/");
	Renderer::getSingleton()->addSearchPath("ExampleExplorer/");
	Renderer::getSingleton()->addSearchPath("ExampleExplorer/shaders/");

	Framework::initialize();

	mExamplesWnd = ImGuiOverlay::ImGuiWindow::root()->createChild<ImGuiOverlay::ImGuiWindow>("examples", true, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar| ImGuiWindowFlags_AlwaysAutoResize);
	mExamplesWnd->drawCallback = [this](auto){
		auto screensize = Renderer::getSingleton()->getSize();
		auto size = ImGui::GetWindowSize();
		ImGui::SetWindowPos({(screensize[0] - size.x) * 0.5f, (screensize[1] - size.y) * 0.5f});

		ImGui::Text("Examples:");
		ImGui::Separator();
		ExampleFrameworkManager::getSingleton().visit([this](auto& name){
			bool b = false;
			ImGui::RadioButton(name.c_str(), & b);
		});

		if (ImGui::Button("load"))
		{
			mExample = ExampleFrameworkManager::getSingleton().product("AtmosphericScatteringExample");
			//for (auto& s: mExampleSelected)
			//	if (s.second)
			//	{
			//		mExample = ExampleFrameworkManager::getSingleton().get(s.first);
			//	}
			mExamplesWnd->visible = false;
		}
		return true;
	};


	mPipeline = Pipeline::Ptr(new ForwardPipleline());

	auto size = getSize();
	resize(size.first, size.second);

}

void ExampleExplorer::updateImpl()
{
	PROFILE("frame update", {});

	static auto lastTime = std::chrono::high_resolution_clock::now();
	auto cur = std::chrono::high_resolution_clock::now();
	mDeltaTime = (cur - lastTime).count() / 1000000000.0f;
	mTime += mDeltaTime;
	lastTime = cur;
	{
		lastTime = cur;
		static float history = 60;
		history = history * 0.99f + (1.0f / mDeltaTime) * 0.01f;

		std::stringstream ss;
		ss.precision(4);
		ss << history << "(" << 1000.0f / history << "ms)";
		if ((int(mTime * 100.0f) % 10) == 0)
			::SetWindowTextA(Renderer::getSingleton()->getWindow(), ss.str().c_str());
	}

	if (mExample)
	{
		mExample->update(mDeltaTime);
		mExample->render();
	}
	else
	{
		mPipeline->execute();
	}

}


int main()
{
	ExampleExplorer e;
	e.init();
	e.update();
	return 0;
}

void ExampleContext::renderScene(Renderer::CommandList::Ref cmdlist, Camera::Ptr cam, UINT, UINT)
{
	auto renderer = Renderer::getSingleton();
	cmdlist->setViewport(cam->viewport);

	cmdlist->setScissorRect({ 0,0, (LONG)cam->viewport.Width, (LONG)cam->viewport.Height });

	cmdlist->setPrimitiveType();

	for (auto& model : mRenderList)
	{
		UINT numMaterials = (UINT)model->materials.size();
		std::vector<std::vector<Mesh::SubMesh*>> subs(numMaterials);

		auto mesh = model->mesh;
		for (auto& sm : mesh->submeshes)
		{
			subs[sm.materialIndex].push_back(&sm);
		}

		for (UINT i = 0; i < numMaterials; ++i)
		{
			auto& material = model->materials[i];
			material->applyTextures();
			auto& pso = material->pipelineState;

			model->visitConstant(Renderer::Shader::ST_VERTEX, i, [&](auto& cbuffer) {
				cbuffer->setVariable("view", &cam->view);
				cbuffer->setVariable("proj", &cam->proj);
				cbuffer->setVariable("world", &model->transform);
				cbuffer->setVariable("nworld", &model->normTransform);
				pso->setVSConstant("VSConstant", cbuffer);
				});

			model->visitConstant(Renderer::Shader::ST_PIXEL, i, [&](auto& cbuffer) {
				cbuffer->setVariable("objpos", &model->aabb.center);
				cbuffer->setVariable("objradius", &model->boundingradius);
				pso->setPSConstant("PSConstant", cbuffer);
				});


			//if (commonConsts)
			//	pso->setPSConstant("CommonConstants", commonConsts);

			cmdlist->setPipelineState(pso);

			for (auto& sm : subs[i])
			{
				cmdlist->setVertexBuffer(mesh->vertices);
				cmdlist->setIndexBuffer(mesh->indices);
				cmdlist->drawIndexedInstanced(sm->numIndices, 1U, sm->startIndex);
			}
		}
	}


}