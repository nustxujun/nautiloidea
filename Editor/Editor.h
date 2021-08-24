#pragma once

#include "Framework.h"
#include "ImguiOverlay.h"
#include "Pipeline.h"
#include "Common.h"
#include "RenderContext.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "World.h"

#include <sol/sol.hpp>

#include "lrdb/server.hpp"

class Editor: public Framework
{
public:
	void init(bool debug_script);
	void updateImpl();
	~Editor();
private:
	LRESULT process(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void initLua(bool debug_script);
	void registerLuaCore(sol::state& state);
	void bindPipelineOperations(sol::state& state);
	void executeScript(std::function<void()>&& call);
	void callScript(sol::object func);

	void updateTime();
	void updateGUI();
	void updateLeftTabBar();
	void updateRightTabBar();
	void updateRes();
	void updateScene();
	void updateSelected();
	void renderScene(Renderer::CommandList * cmdlist, const Pipeline::CameraInfo& cam, UINT flags, UINT mask);
private:
	float mDeltaTime = 0;
	float mTime = 0;

	DirectX::Keyboard mKeyboard;
	DirectX::Mouse mMouse;

	Pipeline::Ptr mPipeline;

	struct 
	{
		SceneObject::Ptr selected = 0;
	}mGuiState;

	sol::state mLuaState;

	std::shared_ptr<lrdb::server> mRemoteDebugServer;
};

