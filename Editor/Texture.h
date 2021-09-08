#pragma once 

#include "Common.h"
#include "AutoObject.h"
#include "Renderer.h"
#include "Resources.h"

class Texture : public AutoObject<Texture>
{
public:
	using Ptr = std::shared_ptr<Texture>;

	Texture(std::string path, bool srgb);

	Renderer::Resource::Ref getDeviceResource()const {return mDeviceResource;}
private:
	Renderer::Resource::Ref mDeviceResource;
	std::string mPath;
};

