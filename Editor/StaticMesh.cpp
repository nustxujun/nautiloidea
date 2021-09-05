#include "StaticMesh.h"
#include "imgui/imgui.h"
#include "World.h"
#include "D3DHelper.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include <regex>
#include <filesystem>


Node::Ptr StaticMeshLoader::operator()(std::string_view filepath)
{

	Assimp::Importer importer;

	const aiScene* scene = importer.ReadFile(filepath.data(),
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		aiProcess_GenNormals |
		//aiProcess_GenSmoothNormals|
		aiProcess_CalcTangentSpace |
		aiProcess_ConvertToLeftHanded);

	if (!scene)
	{
		WARN("cannot load model");
		return 0;
	}

	std::vector<StaticMesh::Ptr> meshes;
	for (uint32_t i = 0; i < scene->mNumMeshes; ++i)
	{
		auto assimp_mesh = scene->mMeshes[i];
		auto static_mesh = parseMesh(assimp_mesh);
		meshes.push_back(static_mesh);
	}


	auto buildScene = [&](auto& buildScene, const aiNode* node)-> Node::Ptr
	{
		auto sn = std::make_shared<Node>();

		for (uint32_t i = 0; i < node->mNumChildren; ++i)
		{
			buildScene(buildScene, node->mChildren[i])->setParent(sn);
		}

		for (uint32_t i = 0; i < node->mNumMeshes; ++i)
		{
			auto mesh = meshes[node->mMeshes[i]];
			sn->addObject(mesh);
		}
		return sn;
	};

	return buildScene(buildScene, scene->mRootNode);
}

StaticMesh::Ptr StaticMeshLoader::parseMesh(aiMesh* aimesh)
{
	using Vector = DirectX::SimpleMath::Vector3;

	auto mesh = std::make_shared<StaticMesh>();
	
	ASSERT(aimesh->HasFaces(), "need face in model");

	UINT ver_size = 0;

	auto make_layout = [&](auto semantic, auto index, auto format)
	{
		mesh->mLayout.push_back({
			semantic, (UINT)index, format, 0, (UINT)ver_size, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
		});
		auto w = (UINT)D3DHelper::sizeof_DXGI_FORMAT(format);
		ver_size += w; 
	};

	make_layout("POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT);
	make_layout("NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT);
	make_layout("NORMAL", 1, DXGI_FORMAT_R32G32B32_FLOAT);
	make_layout("NORMAL", 2, DXGI_FORMAT_R32G32B32_FLOAT);

	if (aimesh->HasTextureCoords(0))
		make_layout("TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT);

	if (aimesh->HasVertexColors(0))
		make_layout("COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM);

	std::vector<char> vertices(aimesh->mNumVertices * ver_size);

	auto numIndices = aimesh->mNumFaces * 3;
	std::vector<int> indices;
	indices.reserve(numIndices);

	Vector pmin = { FLT_MAX,FLT_MAX ,FLT_MAX };
	Vector pmax = { FLT_MIN, FLT_MIN ,FLT_MIN };

	char* buffer = vertices.data();
	char* end = buffer + vertices.size();
	auto write = [&](auto& val){
		auto size = sizeof(val);
		assert(buffer + size <= end);
		memcpy(buffer, &val, size);
		buffer += size;

	};
	for (uint32_t i = 0; i < aimesh->mNumVertices; ++i)
	{
		auto pos = aimesh->mVertices + i;
		write(*pos);
		pmin = Vector::Min(pmin, { pos->x, pos->y, pos->z });
		pmax = Vector::Max(pmax, { pos->x, pos->y, pos->z });

		auto norm = aimesh->mNormals + i;
		write(*norm);

		auto tan = aimesh->mTangents + i;
		write(*tan);

		auto  binorm = aimesh->mBitangents + i;
		write(*binorm);
		
		if (aimesh->HasTextureCoords(0))
		{
			auto uv = aimesh->mTextureCoords[0] + i;
			aiVector2D uv2d = {uv->x, uv->y};
			write(uv2d);
		}

		if (aimesh->HasVertexColors(0))
		{
			auto color = aimesh->mColors[0] + i;
			int uc = ((int)(color->r / 255.0f) << 24) + (int(color->g / 255.0f) << 16) + (int(color->b / 255.0f) << 8) + int(color->a / 255.0f);
			write(uc);
		}
	}

	int* indicesBuffer = indices.data();
	for (uint32_t i = 0; i < aimesh->mNumFaces; ++i)
	{
		for (int k = 0; k < 3; ++k)
			indices.push_back(aimesh->mFaces[i].mIndices[k]);
	}

	auto r = Renderer::getSingleton();
	mesh->mVertices = r->createBuffer((UINT)vertices.size(),ver_size, false, D3D12_HEAP_TYPE_DEFAULT,vertices.data(), (UINT)vertices.size());
	mesh->mIndices = r->createBuffer((UINT)numIndices * 4, 4, false, D3D12_HEAP_TYPE_DEFAULT, indices.data(), numIndices * 4);
	mesh->name = aimesh->mName.C_Str();
	mesh->mIndexCount = numIndices;

	mesh->material = Material::createDefault({Shader::getDefaultVS(), Shader::getDefaultPS()}, mesh->mLayout);
	return mesh;
}

StaticMesh::StaticMesh()
{
	mConstants = Renderer::getSingleton()->createConstantBuffer(sizeof(PrivateConstants));
}


void StaticMesh::onTransformChanged(const DirectX::SimpleMath::Matrix& transform)
{
	PrivateConstants c;
	c.world = transform;
	mConstants->blit(&c,0, sizeof(PrivateConstants));
	
	//material->setVariable(Renderer::Shader::ST_VERTEX, "world", parentNode.lock()->transform);
}


void StaticMesh::draw(Renderer::CommandList * cmdlist)
{
	cmdlist->setPrimitiveType();
	material->getCurrentPipelineStateInstance()->apply(cmdlist);
	//cmdlist->setPipelineState();
	cmdlist->setVertexBuffer(mVertices);
	cmdlist->setIndexBuffer(mIndices);
	cmdlist->drawIndexedInstanced(mIndexCount);
}

