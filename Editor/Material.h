#pragma once 

#include "Common.h"
#include "Resources.h"
#include "Renderer.h"
#include "Texture.h"

class Material: public AutoObject<Material>
{
public:
	union MaterialState
	{
		int64_t value = 0;
		struct
		{
			bool tessellation:1 ;
		};
	} state;

	~Material();
	void refresh(std::vector<Renderer::Shader::Ptr> shaders, const std::vector<D3D12_INPUT_ELEMENT_DESC>& layout);
	Renderer::PipelineState::Ref getCurrentPipelineState()const;
	void updateTextures(Renderer::Shader::ShaderType type, std::map<std::string, Texture::Ptr> textures, bool overwrite = false);
private:
	void refreshTexture();

	std::array<std::map<std::string, Texture::Ptr>, Renderer::Shader::ST_MAX_NUM> mTextures;
	std::map<size_t, Renderer::PipelineState::Ref> mPipelineStates;
	size_t mCurrent;
};


class MaterialResource :public Resource
{
	IS_RESOURCE()
public:
	std::string type() override
	{
		return "Material";
	}

	using Resource::Resource;

	using Textures = std::map<std::string, Resource*>;
	Textures textures;

	using Shaders = std::map<Renderer::Shader::ShaderType, Resource*>;
	Shaders shaders;
};