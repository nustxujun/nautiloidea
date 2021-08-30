#pragma once
#include "Common.h"
#include "AutoObject.h"
#include "Renderer.h"
#include "Resources.h"


class Shader : public AutoObject<Shader>
{
public:
	static Ptr getDefaultVS();
	static Ptr getDefaultPS();
public:
	Shader(Renderer::Shader::Ptr so);
	Renderer::Shader::Ptr getShaderObject() { return mShaderObject; }
	static Ptr load(std::string path);
private:
	Renderer::Shader::Ptr mShaderObject;

};
