#include "Material.h"

//REGISTER_FACTORY(MaterialResource, "mtl","mat")


Material::~Material()
{
	auto r = Renderer::getSingleton();
	for (auto& pso : mPipelineStates)
		r->destroyPipelineState(pso.second);
		

}

Material::Ptr Material::createDefault(std::vector<Shader::Ptr> shaders, const std::vector<D3D12_INPUT_ELEMENT_DESC>& layout)
{
	auto mat = std::make_shared<Material>();
	mat->refresh(std::move(shaders), layout);
	return mat;
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
		auto pso = renderer->createPipelineState(shaderobjs, rs);

		mPipelineStates[hash] = pso;
		mCurrent = hash;

		refreshTexture();
	}

	auto pso = mPipelineStates[mCurrent];
	const std::string private_constants = "PrivateConstants";
	const std::string common_constants = "CommonConstants";
	for (int i = Renderer::Shader::ST_VERTEX; i < Renderer::Shader::ST_MAX_NUM; i++)
	{
		auto type = (Renderer::Shader::ShaderType)i;
		if (pso->hasConstantBuffer(type, private_constants))
		{
			auto cb = pso->createConstantBuffer(type, private_constants);
			mConstants[type] = cb;
			pso->setConstant(type, private_constants, cb);
		}
		pso->setConstant(type, common_constants, getSharedConstants(common_constants));
	}


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

Renderer::ConstantBuffer::Ptr Material::getSharedConstants(const std::string& name)
{
	return getConstants().get(name);
}

Material::SharedConstants& Material::getConstants()
{
	static SharedConstants sharedConstants;
	return sharedConstants;
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

Material::SharedConstants::SharedConstants()
{
	constants["CommonConstants"] = Renderer::getSingleton()->createConstantBuffer(sizeof(CommonConstants));
}

Renderer::ConstantBuffer::Ptr Material::SharedConstants::get(const std::string& name)
{
	return constants.at(name);
}
