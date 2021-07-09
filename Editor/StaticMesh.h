#pragma once 

#include "Common.h"
#include "Resources.h"
#include "Renderer.h"
#include "World.h"
#include "Material.h"


class StaticMeshResource :public Resource
{
	IS_RESOURCE()
public:
	std::string type() override
	{
		return "StaticMesh";
	}

	void interact() override;
	Node::Ptr load();

	using Resource::Resource;
private:
	SceneObject::Ptr parseMesh(struct aiMesh* aimesh);
};

class StaticMesh : public RenderObject
{
	friend class StaticMeshResource;
public:
	void updateConstants(std::function<void(Renderer::PipelineState::Ref)>&& updater) override;
	void draw(Renderer::CommandList * cmdlist) override;



	void changeMeshResource(Resource::Ptr res) ;

private:
	void loadMesh();
private:
	Resource::Ptr mMeshResource ;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mLayout;

	Renderer::Buffer::Ref mVertices;
	Renderer::Buffer::Ref mIndices;
	Material::Ptr mMaterial;
	size_t mIndexCount;
};