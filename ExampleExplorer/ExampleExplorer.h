#pragma once

#include "Framework.h"
#include "ImguiOverlay.h"
#include "ExampleFramework.h"
#include "Pipeline.h"
#include "Common.h"
#include "RenderContext.h"

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
	float mDeltaTime = 0;
	float mTime = 0;
	ExampleFramework::Ptr mExample ;
	ImGuiOverlay::ImGuiWindow* mExamplesWnd;
	Dispatcher mDispatcher;
	ExampleContext mContext;
	Pipeline::Ptr mPipeline;

	std::map<std::string, bool> mExampleSelected;
};

