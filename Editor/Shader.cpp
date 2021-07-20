#include "Shader.h"
#include <fstream>
#include <filesystem>
#include <regex>
REGISTER_FACTORY(ShaderResource, "hlsl")


Shader::Shader(Renderer::Shader::Ptr so)
{
	mShaderObject = so;
}


Shader::Ptr ShaderResource::load()
{
	auto size = std::filesystem::file_size(path);
	std::fstream f(path, std::ios::in);
	std::string content;
	content.resize(size);
	f.read(&content[0],size);
	f.close();

	std::regex pattern( "#define ENTRY_POINT (.+).+#define TARGET (.+)");

	std::smatch result;
	if (!std::regex_match(content, result, pattern))
		WARN("cannot find entry point or target");

	std::string entry_point = result[1];
	std::string target = result[2];

	auto so = Renderer::getSingleton()->compileShaderFromFile(path,entry_point,target);
	return Shader::Ptr(new Shader(so));
}
