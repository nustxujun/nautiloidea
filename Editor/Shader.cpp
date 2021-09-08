#include "Shader.h"
#include <fstream>
#include <filesystem>
#include <regex>

Shader::Ptr Shader::getDefaultVS()
{
	static Ptr vs = Shader::load("scene_vs.hlsl");
	return vs;
}

Shader::Ptr Shader::getDefaultPS()
{
	static Ptr ps = Shader::load("scene_ps.hlsl");
	return ps;
}

Shader::Shader(Renderer::Shader::Ptr so)
{
	mShaderObject = so;
}


Shader::Ptr Shader::load(std::string path)
{
	path = Renderer::getSingleton()->findFile(path);
	auto size = std::filesystem::file_size(path);
	std::fstream f(path, std::ios::in);
	std::string content;
	content.resize(size);
	f.read(&content[0],size);
	f.close();


	std::regex pattern( "#define ENTRY_POINT (.+)\n.*#define TARGET (.+)");

	std::smatch result;
	if (!std::regex_search(content, result, pattern))
		WARN("cannot find entry point or target");

	std::string entry_point = result[1];
	std::string target = result[2];

	auto so = Renderer::getSingleton()->compileShaderFromFile(path,entry_point,target);
	{
		so->registerStaticSampler("point_wrap", D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_WRAP);
		so->registerStaticSampler("point_clamp", D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

		so->registerStaticSampler("linear_wrap", D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP);
		so->registerStaticSampler("linear_clamp", D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

		so->registerStaticSampler("anisotropic_wrap", D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_WRAP);
		so->registerStaticSampler("anisotropic_clamp", D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	}
	return Shader::Ptr(new Shader(so));
}
