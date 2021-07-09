#include "ExampleExplorer.h"
#include "ExampleFramework.h"
#include "Framework.h"
#include "Profile.h"

#include "ProfileWindow.h"
#include "assimp/Importer.hpp"
void ExampleExplorer::init()
{


	char tmp[256] = {};
	::GetCurrentDirectoryA(256,tmp);
	Renderer::getSingleton()->addSearchPath("engine/");
	Renderer::getSingleton()->addSearchPath("engine/Shaders/");
	Renderer::getSingleton()->addSearchPath("ExampleExplorer/");
	Renderer::getSingleton()->addSearchPath("ExampleExplorer/shaders/");
	Renderer::getSingleton()->addSearchPath("ExampleExplorer/resources/");


	Framework::initialize();


	auto mainbar = ImGuiOverlay::ImGuiObject::root()->createChild<ImGuiOverlay::ImGuiMenuBar>(true);
	auto addbutton = [&](const std::string& name, ImGuiOverlay::ImGuiObject* win){
		mainbar->createChild<ImGuiOverlay::ImGuiButton>(name)->callback = [=](auto button) {
			win->visible = !win->visible;
		};
	};

	addbutton("profile", mProfileWindow.getWindow());

	mExamplesWnd = ImGuiOverlay::ImGuiWindow::root()->createChild<ImGuiOverlay::ImGuiWindow>("examples", true, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar| ImGuiWindowFlags_AlwaysAutoResize);
	static int cur_example = 0;
	mExamplesWnd->userdata = int(0);
	mExamplesWnd->drawCallback = [this](auto ui){
		auto screensize = Renderer::getSingleton()->getSize();
		auto size = ImGui::GetWindowSize();
		ImGui::SetWindowPos({(screensize[0] - size.x) * 0.5f, (screensize[1] - size.y) * 0.5f});
		ImGui::Text("Examples:");
		ImGui::Separator();
		int index = 0;
		int* cur = mExamplesWnd->userdata.point<int>();
		std::string selected;
		ExampleFrameworkManager::getSingleton().visit([&](auto& name){
			if (*cur == index)
				selected = name;
			ImGui::RadioButton(name.c_str(), cur, index ++);
			
		});

		if (ImGui::Button("load"))
		{
			mExample = ExampleFrameworkManager::getSingleton().product(selected);
			//mPipeline = mExample->init();
			mPipeline.reset();

			mExamplesWnd->visible = false;
		}
		return true;
	};

	addbutton("examples", mExamplesWnd);


	//mPipeline = Pipeline::Ptr(new ForwardPipleline());

	auto size = getSize();
	resize(size.first, size.second);

	auto p = std::bind(&ExampleExplorer::process, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);

	setProcessor(p);

	mMouse.SetWindow(getWindow());


	//mProfileWindow = ImGuiOverlay::ImGuiObject::root()->createChild<ImGuiOverlay::ImGuiWindow>("profile");


	mPipeline = Pipeline::Ptr(new ForwardPipleline());

	//mExample = ExampleFrameworkManager::getSingleton().product("AtmosphericScatteringExample");
	//mPipeline = mExample->init();
	//mExamplesWnd->visible = false;

}

void ExampleExplorer::updateImpl()
{

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
		if ((int(mTime * 1000.0f) % 16) == 0)
			::SetWindowTextA(Renderer::getSingleton()->getWindow(), ss.str().c_str());
	}

	if (mExample)
	{
		if (!mPipeline)
			mPipeline = mExample->init();

		auto& io = ImGui::GetIO();

		mExample->update(mDeltaTime, mKeyboard, mMouse);
		mExample->render(mPipeline);
	}
	
	{
		PROFILE("execute pipeline", {});

		mPipeline->execute({});
	}
}

LRESULT ExampleExplorer::process(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	mMouse.ProcessMessage(message, wParam, lParam);
	mKeyboard.ProcessMessage(message, wParam, lParam);

	auto mstate = mMouse.GetState();

	auto& io = ImGui::GetIO();
	io.MousePos = { (float)mstate.x, (float)mstate.y};
	io.MouseDown[0] = mstate.leftButton;
	io.MouseDown[1] = mstate.rightButton;
	io.MouseDown[2] = mstate.middleButton;
	static auto lastvalue = mstate.scrollWheelValue;
	io.MouseWheel = (mstate.scrollWheelValue - lastvalue) * 0.01f;
	lastvalue = mstate.scrollWheelValue;

	return DefWindowProcW(hWnd, message, wParam, lParam);
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