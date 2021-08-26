#pragma once 

#include "Common.h"
#include "Resources.h"
#include "Renderer.h"
#include "World.h"
#include "Material.h"


class StaticMesh : public RenderObject
{
	friend class StaticMeshLoader;
public:
	void updateConstants(std::function<void(Renderer::PipelineState::Ref)>&& updater) override;
	void draw(Renderer::CommandList* cmdlist) override;

private:
	Resource::Ptr mMeshResource;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mLayout;

	Renderer::Buffer::Ref mVertices;
	Renderer::Buffer::Ref mIndices;
	Material::Ptr mMaterial;
	size_t mIndexCount;
};

class StaticMeshLoader 
{
public:
	Node::Ptr operator()(std::string_view path);
	std::vector<MaterialResource::Ptr> materials;
private:
	StaticMesh::Ptr parseMesh(struct aiMesh* aimesh);
};

