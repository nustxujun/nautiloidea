#pragma once

#include "Framework.h"
#include "ImguiOverlay.h"
#include "ExampleFramework.h"
#include "Pipeline.h"
#include "Common.h"
#include "RenderContext.h"
#include "Keyboard.h"
#include "Mouse.h"

class ExampleContext final :public RenderContext
{
public:
	void renderScene(Renderer::CommandList::Ref cmdlist, Camera::Ptr cam, UINT flags = 0, UINT mask = 0xffffffff) ;
};

class ExampleExplorer: public Framework
{
public:
	void init();
	void updateImpl();

private:
	LRESULT process(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
private:
	float mDeltaTime = 0;
	float mTime = 0;
	ExampleFramework::Ptr mExample ;
	Pipeline::Ptr mPipeline;
	ImGuiOverlay::ImGuiWindow* mExamplesWnd;
	Dispatcher mDispatcher;
	ExampleContext mContext;

	std::map<std::string, bool> mExampleSelected;

	DirectX::Keyboard mKeyboard;
	DirectX::Mouse mMouse;
};

