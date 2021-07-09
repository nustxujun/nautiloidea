#pragma once 

#include "Common.h"
#include "AutoObject.h"
#include "Renderer.h"
#include "Resources.h"

class  TextureResource : public Resource
{
	IS_RESOURCE()

public:
	std::string type() override
	{
		return "Texture";
	}

	void interact();

};

class Texture : public AutoObject<Texture>
{
public:
	Renderer::Resource::Ref getDeviceResource()const {return mDeviceResource;}
private:
	Renderer::Resource::Ref mDeviceResource;

};

