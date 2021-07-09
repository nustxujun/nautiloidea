#include "Material.h"

REGISTER_FACTORY(MaterialResource, "mtl","mat")

Material::~Material()
{
	auto r = Renderer::getSingleton();
	for (auto& pso : mPipelineStates)
		r->destroyPipelineState(pso.second);
		

}

void Material::refresh(std::vector<Renderer::Shader::Ptr> shaders, const std::vector<D3D12_INPUT_ELEMENT_DESC>& layout)
{
	size_t hash = 0;
	for (auto& s : shaders)
		Common::hash_combine(hash, s->getHash());

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
		return;
	}
	Renderer::RenderState rs = Renderer::RenderState::GeneralSolid;
	rs.setInputLayout(layout);

	if (state.tessellation)
		rs.setPrimitiveType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH);
	else
		rs.setPrimitiveType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

	auto renderer = Renderer::getSingleton();
	auto pso  = renderer->createPipelineState(shaders, rs);

	mPipelineStates[hash] = pso;
	mCurrent = hash;

	refreshTexture();
}

Renderer::PipelineState::Ref Material::getCurrentPipelineState() const
{
	auto ret = mPipelineStates.find(mCurrent);
	if (ret == mPipelineStates.end())
		return {};
	return ret->second;
}

void Material::updateTextures(Renderer::Shader::ShaderType type,std::map<std::string, Texture::Ptr> textures, bool overwrite)
{
	if (overwrite)
		mTextures[type].swap(textures);
	else
	{
		for (auto& t: textures)
			mTextures[type][t.first] = t.second;
	}
	refreshTexture();
}

void Material::refreshTexture()
{
	auto& pso = mPipelineStates[mCurrent];


	for (int i = 0; i < Renderer::Shader::ST_MAX_NUM; ++i)
	{
		auto& textures = mTextures[i];
		for (auto& item : textures)
		{
			pso->setResource((Renderer::Shader::ShaderType)i, item.first, item.second->getDeviceResource()->getShaderResource());
		}
	}

}
