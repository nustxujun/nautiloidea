#include "Editor.h"
#include "Framework.h"
#include "Profile.h"

#include "assimp/Importer.hpp"

#include "Resources.h"
#include "World.h"

#include "EditorLuaBindings.h"
#include "imgui_lua_binding.hpp"
#include "filesystem_lua_binding.hpp"

static void initImGui()
{
	ImGuiIO& io = ImGui::GetIO();
	io.KeyMap[ImGuiKey_Tab] = VK_TAB;
	io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
	io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
	io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
	io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
	io.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
	io.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
	io.KeyMap[ImGuiKey_Home] = VK_HOME;
	io.KeyMap[ImGuiKey_End] = VK_END;
	io.KeyMap[ImGuiKey_Insert] = VK_INSERT;
	io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
	io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
	io.KeyMap[ImGuiKey_Space] = VK_SPACE;
	io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
	io.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
	io.KeyMap[ImGuiKey_KeyPadEnter] = VK_RETURN;
	io.KeyMap[ImGuiKey_A] = 'A';
	io.KeyMap[ImGuiKey_C] = 'C';
	io.KeyMap[ImGuiKey_V] = 'V';
	io.KeyMap[ImGuiKey_X] = 'X';
	io.KeyMap[ImGuiKey_Y] = 'Y';
	io.KeyMap[ImGuiKey_Z] = 'Z';
	ImGuiPass::getInstance();
}

void Editor::init(bool debug_script)
{

	// search path
	char tmp[256] = {};
	::GetCurrentDirectoryA(256,tmp);
	Renderer::getSingleton()->addSearchPath("engine/");
	//Renderer::getSingleton()->addSearchPath("engine/Shaders/");
	Renderer::getSingleton()->addSearchPath("Editor/");
	Renderer::getSingleton()->addSearchPath("Editor/shaders/");
	Renderer::getSingleton()->addSearchPath("resources/");
	Renderer::getSingleton()->addSearchPath("resources/shaders/");

	// framework
	Framework::initialize();

	auto size = getSize();
	resize(size.first, size.second);

	// input
	auto p = std::bind(&Editor::process, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
	setProcessor(p);
	mMouse.SetWindow(getWindow());

	auto pp = new ForwardPipleline;
	mPipeline = Pipeline::Ptr(pp);
	//pp->setUICallback(std::bind(&Editor::updateGUI, this));
	pp->init();
	initImGui();

	ResourceSystem::getInstance().refresh();
	World::getInstance().newWorld();

	initLua(debug_script);


	//auto constants = Material::getSharedConstants("CommonConstants");

	//using M = DirectX::SimpleMath::Matrix;
	//M view = M::CreateLookAt({ 0,0,-50 }, { 0,0,0 }, { 0,1,0 }).Transpose();
	//M proj = M::CreatePerspectiveFieldOfView(0.75, float(size.first) / float(size.second), 0.1f, 100.0f).Transpose();

	//CommonConstants c;
	//memcpy(&c.view, &view, sizeof(c.view));
	//memcpy(&c.proj, &proj, sizeof(c.proj));
	//constants->blit(&c);
}

Editor::~Editor()
{
	setProcessor(0);
}

void Editor::initLua(bool debug_script)
{
	mLuaState.open_libraries(
		sol::lib::base, 
		sol::lib::package,
		sol::lib::debug, 
		sol::lib::string,
		sol::lib::table,
		sol::lib::coroutine,
		sol::lib::io,
		sol::lib::math,
		sol::lib::count);


	ImGuiLuaBinding::bind(mLuaState);
	EditorLuaBinding::bindMath(mLuaState);
	EditorLuaBinding::bindWorld(mLuaState);
	EditorLuaBinding::bindRender(mLuaState);
	bindPipelineOperations(mLuaState);


	LuaFileSystem::bind(mLuaState); 
	  

	registerLuaCore(mLuaState);

	mLuaState.add_package_loader([](sol::this_state s, std::string path){
		sol::state_view state(s);
		auto ret = state.load_file(std::string("resources/scripts/") + path + ".lua");
		if (ret.valid()) {
			return ret.get<sol::object>();
		}
		else {
			sol::error err = ret;
			return sol::make_object(state, err.what());
		}
	});

	if (debug_script)
	{
		mRemoteDebugServer = decltype(mRemoteDebugServer)(new lrdb::server(21110));
		mRemoteDebugServer->reset(mLuaState);
	}

	executeScript([&]() {
		mLuaState.restart_gc();
		mLuaState.safe_script_file("resources/scripts/main.lua");
	});
}

void Editor::registerLuaCore(sol::state& state)
{
	auto core = state.create_table();
	state["core"] = core;

	core["get_window_size"] = [=](sol::this_state L){
		auto size = getSize();
		return sol::as_returns(std::vector<int>{size.first, size.second});
	};

	core["input"] = [](){
		std::string cont;
		std::getline(std::cin, cont);
		return cont;
	};

	core["reload"] = [&](){
		mLuaState.restart_gc();
		mLuaState.safe_script_file("resources/scripts/main.lua");
	};

	Renderer::getSingleton()->registerRenderEvent(Renderer::RE_BEFORE_RESIZE, [=](auto e) {
		executeScript([&]() {
			auto obj = mLuaState["core"]["window_resize_callback"];
			if (obj.valid())
				callScript(obj);
		});
	});

}

void Editor::executeScript(std::function<void()>&& call)
{
	while (true)
	{
		try
		{
			call();
			break;
		}
		catch (std::exception& e)
		{
			std::cout << e.what() << std::endl;
			/*auto ret = mLuaState.safe_script_file("resources/scripts/debugger.lua");
			ret.get<sol::table>()["start"]();*/
		}
	}
}

void Editor::callScript(sol::object func)
{
	sol::protected_function obj = func;
	if (obj.valid())
	{
		auto ret = obj();
		if (!ret.valid())
		{
			sol::error err = ret;
			throw std::exception(err.what());
		}
	}
	else
		throw std::exception("failed to call script function");
}


void Editor::bindPipelineOperations(sol::state& state)
{


}



void Editor::updateTime()
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
	mPipeline->addRenderScene(std::bind(&Editor::renderScene, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
}

void Editor::updateGUI()
{


	//updateLeftTabBar();
	//updateRightTabBar();
	//ImGui::ShowDemoWindow();
}

void Editor::updateScene()
{
	auto buildtree = [&state = mGuiState](auto build, Node::Ptr node)->void
	{
		//if (ImGui::TreeNodeEx(node, 0, ""))
		{
			for (auto& o : node->objects)
			{
				bool selected = o == state.selected;
				if ( ImGui::Selectable(o->name.c_str(),selected))
					state.selected = o;
			}

			for (auto& n : node->children)
			{
				build(build, n);
			}
			//ImGui::TreePop();
		}
	};
	buildtree(buildtree, World::getInstance().getRoot());
}

void Editor::updateSelected()
{
	if (!mGuiState.selected)
		return;
	ImGui::Text("Name: %s",mGuiState.selected->name.c_str());
	//ImGui::Text("Type: %s", mGuiState.selected->);

}

void Editor::updateLeftTabBar()
{
	auto size = getSize();
	ImGui::Begin("left", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_NoMove);
	ImGui::SetWindowSize({ 200.0f, float(size.second) }, ImGuiCond_Always);
	ImGui::SetWindowPos({ 0,0 }, ImGuiCond_Always);

	if (ImGui::BeginTabBar("leftTabBar", ImGuiTabBarFlags_None))
	{
		if (ImGui::BeginTabItem("resources"))
		{
			updateRes();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("scene"))
		{
			updateScene();
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
	ImGui::End();
}

void Editor::updateRightTabBar()
{
	auto size = getSize();
	ImGui::Begin("right", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_NoMove);
	ImGui::SetWindowSize({ 200.0f, float(size.second) }, ImGuiCond_Always);
	ImGui::SetWindowPos({ size.first - 200.0f,0 }, ImGuiCond_Always);

	if (ImGui::BeginTabBar("leftTabBar", ImGuiTabBarFlags_None))
	{
		if (ImGui::BeginTabItem("property"))
		{
			updateSelected();
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
	ImGui::End();
}

void Editor::updateRes()
{
	auto& resourceTree = ResourceSystem::getInstance().getResources();
	auto& reslist = ResourceSystem::getInstance().getResourceList();



	static char findresult[256] = {0};
	ImGui::InputText("find",findresult,256,0);
	static int selectedFilter = 0;
	static std::vector<std::string> filterlist; 
	filterlist.clear();
	filterlist.push_back("All");
	auto tl = ResourceSystem::getInstance().getTypeList();
	filterlist.insert(filterlist.end(), tl.begin(), tl.end());
	if (ImGui::BeginCombo("filter", filterlist[selectedFilter].c_str()))
	{
		for (int i = 0; i < filterlist.size(); ++i)
		{
			bool selected = selectedFilter == i ;
			if (ImGui::Selectable(filterlist[i].c_str(), selected))
				selectedFilter = i;
			if (selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	struct 
	{
	
		void buildFolder(const std::filesystem::path& name,const ResourceSystem::Directory& dir)
		{
			if (!ImGui::TreeNode(name.filename().string().c_str()))
				return;
			for (auto& s: dir.subs)
				buildFolder(s.first,s.second);

			for (auto& s : dir.resources)
				buildRes(s.first, s.second);
				 
			ImGui::TreePop();
		}
		void buildRes(const std::filesystem::path& name, const Resource::Ptr& res)
		{
			if (selectedFilter != 0 && res->type() != filterlist[selectedFilter])
				return ;

			auto filename = name.filename().string();
			auto popupname = filename + "_popup";
			ImGui::Text("[%s]",res->type().c_str());
			ImGui::SameLine();
			if (ImGui::Selectable(  filename.c_str()))
			{
				ImGui::OpenPopup(popupname.c_str());

			}

			if (ImGui::BeginPopup(popupname.c_str()))
			{
				ImGui::Text(filename.c_str());
				ImGui::Separator();
				res->interact();

				ImGui::EndPopup();
			}
		}

	}builder;

	std::string lowerret = findresult;
	std::transform(lowerret.begin(), lowerret.end(), lowerret.begin(), ::tolower);

	if (strlen(findresult) != 0)
	{
		for (auto& i : reslist)
		{
			auto resname = i->getPath().filename().string();
			std::string lowername = resname;
			std::transform(resname.begin(), resname.end(), lowername.begin(), ::tolower);
			if (lowername.find(lowerret) != std::string::npos)
			{
				builder.buildRes(resname, i);
			}
		}
	}
	else
	{
		for (auto& s : resourceTree)
		{
			builder.buildFolder(s.first, s.second);
		}
	}

}

void Editor::renderScene(Renderer::CommandList * cmdlist, const Pipeline::CameraInfo& cam, UINT flags, UINT mask)
{
	auto& world = World::getInstance();
	std::vector<RenderObject*> ros;
	world.visitRenderable([&ros](RenderObject::Ptr ro){
		ros.push_back((RenderObject*)ro.get());
	});

	for (auto& ro : ros)
	{


		ro->draw(cmdlist);
	}
}

void Editor::updateImpl()
{

	updateTime();
	executeScript([&]()
		{
			callScript(mLuaState["core"]["update_callback"]);
		}
	);

	ImGuiPass::getInstance()->ready();
	//mPipeline->execute({});
}

static bool ImGui_ImplWin32_UpdateMouseCursor()
{
	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
		return false;

	ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
	if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor)
	{
		// Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
		::SetCursor(NULL);
	}
	else
	{
		// Show OS mouse cursor
		LPTSTR win32_cursor = IDC_ARROW;
		switch (imgui_cursor)
		{
		case ImGuiMouseCursor_Arrow:        win32_cursor = IDC_ARROW; break;
		case ImGuiMouseCursor_TextInput:    win32_cursor = IDC_IBEAM; break;
		case ImGuiMouseCursor_ResizeAll:    win32_cursor = IDC_SIZEALL; break;
		case ImGuiMouseCursor_ResizeEW:     win32_cursor = IDC_SIZEWE; break;
		case ImGuiMouseCursor_ResizeNS:     win32_cursor = IDC_SIZENS; break;
		case ImGuiMouseCursor_ResizeNESW:   win32_cursor = IDC_SIZENESW; break;
		case ImGuiMouseCursor_ResizeNWSE:   win32_cursor = IDC_SIZENWSE; break;
		case ImGuiMouseCursor_Hand:         win32_cursor = IDC_HAND; break;
		case ImGuiMouseCursor_NotAllowed:   win32_cursor = IDC_NO; break;
		}
		::SetCursor(::LoadCursor(NULL, win32_cursor));
	}
	return true;
}

#ifndef DBT_DEVNODES_CHANGED
#define DBT_DEVNODES_CHANGED 0x0007
#endif

LRESULT Editor::ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui::GetCurrentContext() == NULL)
		return 0;

	ImGuiIO& io = ImGui::GetIO();
	switch (msg)
	{
	case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
	case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK:
	{
		int button = 0;
		if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK) { button = 0; }
		if (msg == WM_RBUTTONDOWN || msg == WM_RBUTTONDBLCLK) { button = 1; }
		if (msg == WM_MBUTTONDOWN || msg == WM_MBUTTONDBLCLK) { button = 2; }
		if (msg == WM_XBUTTONDOWN || msg == WM_XBUTTONDBLCLK) { button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? 3 : 4; }
		if (!ImGui::IsAnyMouseDown() && ::GetCapture() == NULL)
			::SetCapture(hwnd);
		io.MouseDown[button] = true;
		return 0;
	}
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
	case WM_XBUTTONUP:
	{
		int button = 0;
		if (msg == WM_LBUTTONUP) { button = 0; }
		if (msg == WM_RBUTTONUP) { button = 1; }
		if (msg == WM_MBUTTONUP) { button = 2; }
		if (msg == WM_XBUTTONUP) { button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? 3 : 4; }
		io.MouseDown[button] = false;
		if (!ImGui::IsAnyMouseDown() && ::GetCapture() == hwnd)
			::ReleaseCapture();
		return 0;
	}
	case WM_MOUSEWHEEL:
		io.MouseWheel += (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
		return 0;
	case WM_MOUSEHWHEEL:
		io.MouseWheelH += (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
		return 0;
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		if (wParam < 256)
			io.KeysDown[wParam] = 1;
		return 0;
	case WM_KEYUP:
	case WM_SYSKEYUP:
		if (wParam < 256)
			io.KeysDown[wParam] = 0;
		return 0;
	case WM_CHAR:
		// You can also use ToAscii()+GetKeyboardState() to retrieve characters.
		if (wParam > 0 && wParam < 0x10000)
			io.AddInputCharacterUTF16((unsigned short)wParam);
		return 0;
	case WM_SETCURSOR:
		if (LOWORD(lParam) == HTCLIENT && ImGui_ImplWin32_UpdateMouseCursor())
			return 1;
		return 0;
	//case WM_DEVICECHANGE:
	//	if ((UINT)wParam == DBT_DEVNODES_CHANGED)
	//		g_WantUpdateHasGamepad = true;
	//	return 0;
	}
	return 0;
}

LRESULT Editor::process(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	auto& io = ImGui::GetIO();
	auto mstate = mMouse.GetState();
	io.MousePos = { (float)mstate.x, (float)mstate.y };

	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
		return true;

	mMouse.ProcessMessage(message, wParam, lParam);
	mKeyboard.ProcessMessage(message, wParam, lParam);



	//io.MouseDown[0] = mstate.leftButton;
	//io.MouseDown[1] = mstate.rightButton;
	//io.MouseDown[2] = mstate.middleButton;
	//static auto lastvalue = mstate.scrollWheelValue;
	//io.MouseWheel = (mstate.scrollWheelValue - lastvalue) * 0.01f;
	//lastvalue = mstate.scrollWheelValue;

	
	return DefWindowProcW(hWnd, message, wParam, lParam);
}


int main(int argn, char** args)
{
	bool debug_script = false;
	const char debug[] = "-debug_script"; 
	for (int i = 1; i < argn; ++i)
	{
		if (strncmp(debug, args[i], sizeof(debug)) == 0)
			debug_script = true;
	}
	Editor e;
	e.init(debug_script);
	e.update();
	return 0;
}

