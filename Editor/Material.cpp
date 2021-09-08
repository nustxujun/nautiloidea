#include "Material.h"

//REGISTER_FACTORY(MaterialResource, "mtl","mat")


Material::~Material()
{
}

Material::Ptr Material::createDefault(std::vector<Shader::Ptr> shaders, const std::vector<D3D12_INPUT_ELEMENT_DESC>& layout)
{
	auto mat = std::make_shared<Material>();
	mat->refresh(std::move(shaders), layout);
	return mat;
}

void Material::setConstants(Renderer::Shader::ShaderType type, const std::string& name, Renderer::ConstantBuffer::Ptr constants)
{
	mConstants[type][name] = constants;
}


void Material::refresh(std::vector<Shader::Ptr> shaders, const std::vector<D3D12_INPUT_ELEMENT_DESC>& layout)
{
	size_t hash = 0;
	std::vector<Renderer::Shader::Ptr> shaderobjs;
	for (auto& s : shaders)
	{
		auto so = s->getShaderObject();
		shaderobjs.push_back(so);
		Common::hash_combine(hash, so->getHash());
	}
	for (auto& l : layout)
	{
		Common::hash_combine(hash, l.SemanticName);
		Common::hash_combine(hash, l.SemanticIndex);
		Common::hash_combine(hash, l.Format);
		Common::hash_combine(hash, l.InputSlot);
		Common::hash_combine(hash, l.AlignedByteOffset);
		Common::hash_combine(hash, l.InputSlotClass);
		Common::hash_combine(hash, l.InstanceDataStepRate);
	}
	Common::hash_combine(hash, state.value);
	if (mPipelineStates.find(hash) != mPipelineStates.end())
	{
		mCurrent = hash;
	}
	else
	{
		Renderer::RenderState rs = Renderer::RenderState::GeneralSolid;
		rs.setInputLayout(layout);

		if (state.tessellation)
			rs.setPrimitiveType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH);
		else
			rs.setPrimitiveType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

		auto renderer = Renderer::getSingleton();

		mPipelineStates[hash] = std::make_shared<Renderer::PipelineStateInstance>( rs, shaderobjs);
		mCurrent = hash;

	}

	auto pso = mPipelineStates[mCurrent];
	//const std::string material_constants = "MaterialConstants";
	//for (int i = Renderer::Shader::ST_VERTEX; i < Renderer::Shader::ST_MAX_NUM; i++)
	//{
	//	auto type = (Renderer::Shader::ShaderType)i;
	//	if (pso->hasConstantBuffer(type, material_constants))
	//	{
	//		auto cb = pso->createConstantBuffer(type, material_constants);
	//		mConstants[type] = cb;
	//		//pso->setConstant(type, material_constants, cb);
	//	}
	//}


}

Renderer::PipelineStateInstance::Ptr Material::getCurrentPipelineStateInstance() const
{
	auto ret = mPipelineStates.find(mCurrent);
	if (ret == mPipelineStates.end())
		return {};
	return ret->second;
}

void Material::setTexture(Renderer::Shader::ShaderType type, const std::string& name, Texture::Ptr tex)
{
	mTextures[type][name] = tex;
}

void Material::updateCommandList(Renderer::CommandList* cmdlist)
{
	auto cur = mPipelineStates[mCurrent];
	cmdlist->setPipelineState(cur->getPipelineState());
	for (auto& c : mConstants)
	{
		for (auto& h : c.second)
		{
			cmdlist->setRootDescriptorTable(cur->getConstantBufferSlot(c.first, h.first), h.second->getHandle());
		}
	}

	for (auto& ts : mTextures)
	{
		for (auto& t : ts.second)
		{
			cmdlist->setRootDescriptorTable(cur->getResourceSlot(ts.first, t.first), t.second->getDeviceResource()->getShaderResource());
		}
	}
}
