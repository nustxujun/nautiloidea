#pragma once 

#include "Common.h"
#include "Resources.h"
#include "Renderer.h"
#include "Texture.h"
#include "Shader.h"

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
	void refresh(std::vector<Shader::Ptr> shaders, const std::vector<D3D12_INPUT_ELEMENT_DESC>& layout);
	Renderer::PipelineStateInstance::Ptr getCurrentPipelineStateInstance()const;

	static Ptr createDefault(std::vector<Shader::Ptr> shaders, const std::vector<D3D12_INPUT_ELEMENT_DESC>& layout);
	void setConstants(Renderer::Shader::ShaderType type, const std::string& name, Renderer::ConstantBuffer::Ptr constants);

	void updateCommandList(Renderer::CommandList* cmdlist);

	void setTexture(Renderer::Shader::ShaderType type, const std::string& name, Texture::Ptr tex);

	static Renderer::PipelineStateInstance::Ptr getSharedPipelineStateInstance(const Renderer::RenderState& rs, const std::vector<Shader::Ptr>& shaders);
private:


	std::unordered_map<Renderer::Shader::ShaderType,std::map<std::string, Texture::Ptr>> mTextures;
	std::unordered_map<size_t, Renderer::PipelineStateInstance::Ptr> mPipelineStates;
	std::unordered_map<Renderer::Shader::ShaderType, std::unordered_map<std::string, Renderer::ConstantBuffer::Ptr>> mConstants;
	size_t mCurrent;
};




//class MaterialResource :public Resource
//{
//	IS_RESOURCE()
//public:
//	using Ptr = std::shared_ptr<MaterialResource>;
//	std::string type() override
//	{
//		return "Material";
//	}
//
//
//	using Textures = std::map<std::string, Resource*>;
//	Textures textures;
//
//	using Shaders = std::map<Renderer::Shader::ShaderType, ShaderResource*>;
//	Shaders shaders;
//};