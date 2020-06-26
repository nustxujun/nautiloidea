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
			//mPipeline = mExample->init();
			mPipeline.reset();

			mExamplesWnd->visible = false;
		}
		return true;
	};


	//mPipeline = Pipeline::Ptr(new ForwardPipleline());

	auto size = getSize();
	resize(size.first, size.second);

	auto p = std::bind(&ExampleExplorer::process, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);

	setProcessor(p);

	mMouse.SetWindow(getWindow());


	mProfileWindow = ImGuiOverlay::ImGuiObject::root()->createChild<ImGuiOverlay::ImGuiWindow>("profile");
	mProfileWindow->drawCallback = [](auto gui) {
		static bool show_inactive = false;
		static size_t selecteditem = -1;
		static std::vector<float> times(100,0);
		static float upper = 0;
		static int interval = 4;
		bool resetProfile = false;
		ImGui::Checkbox("show inactive pass", &show_inactive);
		if (ImGui::Button("reset max"))
		{
			resetProfile = true;
			upper = 0;
		}
		static int record_count = 0;

		ImGui::SliderInt("interval", &interval,1,128);
		ImGui::PlotHistogram("timeline",times.data(), (int)times.size(), 0, "", 0, upper,ImVec2{0,100});
		float width = ImGui::GetWindowWidth();
		times.resize((size_t)(width * 0.125f));
		struct Node
		{
			size_t index = 0;
			const char* name;
			float cpu = 0;
			float cpumax = 0;
			float gpu = 0;
			float gpumax = 0;
			Node* parent = 0;
			std::vector<Node> children;
			bool active = false;
			bool selected = false;
			void draw()
			{
				
				if (!show_inactive && !active)
					return;

				bool allinvisible = true;
				for (auto& c:children)
					if (c.active)
						allinvisible = false;

				std::string format = "%s | cpu: %.3f(%.3f)";
				if (gpu >= 0.001f)
					format += " | gpu: %.3f(%.3f)";

				if (selecteditem == index)
				{
					if (record_count++ % interval == 0)
					{
						times.erase(times.begin());
						times.push_back(cpu);
						//upper = std::max(upper, cpu);
						if (cpu > upper)
							upper = (upper * 0.9f + cpu * 0.1f) ;
					}
				}

				if (children.empty() || (!show_inactive && allinvisible))
				{
					if (selecteditem == index)
					{
						ImGui::Selectable("", true);
						ImGui::SameLine();
					}
					ImGui::BulletText("");
					if (ImGui::IsItemClicked())
					{
						selecteditem = index;
					}
					ImGui::SameLine();
			
					ImGui::TextColored(active ? ImVec4{ 1,1,1,1 } : ImVec4{ 0.5,0.5,0.5,1 }, format.c_str(), name, cpu, cpumax, gpu, gpumax);
					if (ImGui::IsItemClicked())
					{
						selecteditem = index;
					}
				}
				else 
				{
	
					bool bopen = ImGui::TreeNodeEx(name, selecteditem == index ? ImGuiTreeNodeFlags_Selected: 0,"");
					ImGui::SameLine();
					ImGui::TextColored(active ? ImVec4{ 1,1,1,1 } : ImVec4{ 0.5,0.5,0.5,1 }, format.c_str(), name, cpu, cpumax, gpu, gpumax);
					if (ImGui::IsItemClicked())
					{
						selecteditem = index;
					}
					if (!bopen)
						return;
					for (auto& c: children)
						c.draw();
					ImGui::TreePop();
				}
			}
		};

		Node root;
		root.cpu = 0;
		root.name = "all";
		root.active = true;
		Node* parent = &root;
		Node* current = 0;
		size_t curDepth = 0;
		auto cur = std::chrono::high_resolution_clock::now();
		size_t inc_index = 1;
		ProfileMgr::Singleton.visit([&](ProfileMgr::Node* node, size_t depth){
			if (curDepth > depth)
			{
				while (curDepth > depth)
				{
					parent = parent->parent;
					curDepth--;
				}
			}
			else if (curDepth < depth)
			{
				parent = current;
				curDepth = depth;
			}

			parent->children.push_back({});
			Node& n = parent->children.back();
			auto profile = node->profile;
			if (resetProfile)
				profile->reset();
			n.index = inc_index++;
			n.name = node->name.c_str();
			n.cpu = profile->getCPUTime();
			n.cpumax = profile->getCPUMax();
			n.gpu = profile->getGPUTime();
			n.gpumax = profile->getGPUMax();
			n.parent = parent;
			n.active = (cur - node->timestamp).count() < 1000000000;
			current = &n;
		});

		root.draw();

		return true;
	};

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
		mExample->update(mDeltaTime, mKeyboard, mMouse);
		mExample->render(mPipeline);
	}
	
	{
		PROFILE("execute pipeline", {});

		mPipeline->execute();
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