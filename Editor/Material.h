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
	Renderer::PipelineState::Ref getCurrentPipelineState()const;
	void updateTextures(Renderer::Shader::ShaderType type, std::map<std::string, Texture::Ptr> textures, bool overwrite = false);

	static Ptr createDefault(std::vector<Shader::Ptr> shaders, const std::vector<D3D12_INPUT_ELEMENT_DESC>& layout);

	template<class T>
	void setVariable(Renderer::Shader::ShaderType type, std::string name, const T& val)
	{
		ASSERT(mConstants.find(type) != mConstants.end(), "material need to init");
		mConstants[type]->setVariable<T>(name, val);
	}

	static Renderer::ConstantBuffer::Ptr getSharedConstants(const std::string& name);
private:
	struct SharedConstants
	{
		SharedConstants();
		Renderer::ConstantBuffer::Ptr get(const std::string& name);

		std::map<std::string, Renderer::ConstantBuffer::Ptr> constants;
	};
	static SharedConstants& getConstants();
private:

	

	void refreshTexture();

	std::array<std::map<std::string, Texture::Ptr>, Renderer::Shader::ST_MAX_NUM> mTextures;
	std::map<size_t, Renderer::PipelineState::Ref> mPipelineStates;
	std::map<Renderer::Shader::ShaderType, Renderer::ConstantBuffer::Ptr> mConstants;
	size_t mCurrent;
};


struct LightInfo
{
	float4 pos;
	float4 dir;
	float4 color;
};
struct CommonConstants
{
	matrix view;
	matrix proj;

	// camera
	float4 campos;
	float4 camdir;

	// lights
	LightInfo lights[4];

	int numlights;
	float3 sundir; // directional light dir

	float4 suncolor;// directional light color

	float deltatime;
	float time;
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