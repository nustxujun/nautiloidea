#pragma once
#include "Common.h"
#include "AutoObject.h"
#include "Renderer.h"
#include "Resources.h"


class Shader : public AutoObject<Shader>
{
public:
	Shader(Renderer::Shader::Ptr so);
	Renderer::Shader::Ptr getShaderObject() { return mShaderObject; }
private:
	Renderer::Shader::Ptr mShaderObject;

};

class ShaderResource : public Resource
{
	IS_RESOURCE()

public:
	std::string type() override
	{
		return "Shader";
	}


	Shader::Ptr load();

	std::string path;
};


