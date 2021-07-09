#pragma once

#include "Profile.h"
#include "ImguiOverlay.h"
class ProfileWindow
{
public:
	ProfileWindow();
	~ProfileWindow();


	ImGuiOverlay::ImGuiObject* getWindow(){return mProfileWindow;}
private:
	ImGuiOverlay::ImGuiObject* mProfileWindow;
};