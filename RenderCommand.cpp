#ifdef _CONSOLE

#include "RenderCommand.h"
#include "RenderContext.h"
#include "D3DHelper.h"
#include <iostream>



void CommandReceiver::init(bool host)
{
	if (host)
		mIPC.listen("renderstation");
	else
		mIPC.connect("renderstation");
}

void CommandReceiver::receive()
{
	std::map<std::string, std::function<bool(void)>> processors;

	processors["createMesh"] = [&ipc = mIPC]() {
		UINT32 numBytesVertices, numBytesIndices, numVertices, numIndices, verticesStride, indicesStride;
		std::vector<char> vertices, indices;
		
		std::string name;
		ipc >> name;

		ipc >> numBytesVertices >> numVertices >> verticesStride;
		vertices.resize(numBytesVertices);
		ipc.receive(vertices.data(), numBytesVertices);

		ipc >> numBytesIndices >> numIndices >> indicesStride;
		indices.resize(numBytesIndices);
		ipc.receive(indices.data(), numBytesIndices);

		UINT numSubmeshs;
		ipc  >> numSubmeshs;

		struct SubMesh
		{
			UINT materialIndex;
			UINT startIndex;
			UINT numIndices;
		};

		std::vector<SubMesh> submeshs(numSubmeshs);

		for (auto& sm:submeshs)
			ipc >> sm;

		auto context = RenderContext::getSingleton();
		auto mesh = context->createObject<Mesh>(name);

		mesh->init(vertices,indices,verticesStride, indicesStride, numIndices);
		for (auto& sm:submeshs)
			mesh->submeshes.push_back({
				sm.materialIndex,
				sm.startIndex,
				sm.numIndices,
			});
		return true;
	};

	processors["createTexture"] = [&ipc = mIPC]() {
		UINT32 width, height, size;
		DXGI_FORMAT fmt;
		bool srgb;
		std::string name;
		ipc >> name >>  width >> height >> fmt >> srgb >> size ;
		std::vector<char> data;
		data.resize(size);

		ipc.receive(data.data(), size);

		auto context = RenderContext::getSingleton();
		auto tex = context->createObject<Texture>(name);
		tex->init(width,height, fmt, data.data(),srgb);
		return true;
	};

	processors["createModel"] = [&ipc = mIPC]() {
		auto context = RenderContext::getSingleton();
		std::string name;
		ipc >> name;
		auto model = context->createObject<Model>(name);

		UINT32 meshcount;
		ipc >> meshcount;
		std::string meshname;
		ipc >> meshname;
		auto mesh = context->getObject<Mesh>(meshname);
		model->mesh =  mesh;

		ipc >> model->transform >> model->normTransform >> model->aabb;
		UINT  numMaterials;
		ipc >> numMaterials;
		for (UINT i = 0; i < numMaterials; ++i)
		{
			std::string matname;
			ipc >> matname;
			model->materials.push_back(context->getObject<Material>(matname));
		}

		model->init();
		return true;
	};

	processors["createMaterial"] = [&ipc = mIPC]() {
		auto context = RenderContext::getSingleton();
		std::string name;
		ipc >> name;
		auto material = context->createObject<Material>(name);
		std::string vs, ps, pscontent;
		ipc >> vs >> ps >> pscontent;

		UINT32 count;
		ipc >> count;
		for (UINT32 i = 0; i < count; ++i)
		{
			std::string texname;
			ipc >>  texname;

			material->textures[texname] = context->getObject<Texture>(texname);
		}

		material->init(vs, ps, pscontent);
		return true;
	};

	processors["createCamera"] = [&ipc = mIPC]() {
		auto context = RenderContext::getSingleton();
		std::string name;
		ipc >> name;
		auto cam = context->createObject<Camera>(name);
		ipc >>cam->pos >> cam->dir >> cam->view >> cam->proj 
			>> cam->viewport.TopLeftX >> cam->viewport.TopLeftY >> cam->viewport.Width  >> cam->viewport.Height >> cam->viewport.MinDepth >> cam->viewport.MaxDepth;
		return true;
	};

	processors["createLight"] = [&ipc = mIPC]() {
		auto context = RenderContext::getSingleton();
		std::string name;
		ipc >> name;
		auto light = context->createObject<Light>(name);
		ipc >> light->type >> light->color >> light->dir;
		return true;
	};

	processors["createReflectionProbe"] = [&ipc = mIPC]() {
		auto context = RenderContext::getSingleton();
		std::string name;
		ipc >> name;
		auto probe = context->createObject<ReflectionProbe>(name);

		ipc >> probe->transform >> probe->influence >> probe->brightness;
		UINT cubesize, size;
		ipc >> cubesize >> size;
		std::vector<char> data(size);
		ipc.receive(data.data(),size);
		probe->init(cubesize, data.data(), size);
		return true;
	};

	processors["createSky"] = [&ipc = mIPC]() {
		auto context = RenderContext::getSingleton();
		std::string name, mesh, material;
		Matrix world;
		ipc >> name >> mesh >> material >> world;

		auto env = context->createObject<Sky>(name);
		env->mesh = context->getObject<Mesh>(mesh);
		env->material = context->getObject<Material>(material);
		env->transform = world;
		ipc >> env->aabb;

		env->init();
		return true;
	};

	processors["done"] = []() {
		
		auto context = RenderContext::getSingleton();

		std::vector<ReflectionProbe::Ptr> probes;
		context->visiteObjects<ReflectionProbe>([&](ReflectionProbe::Ptr probe) {
			probes.push_back(probe);
		});
		ReflectionProbe::initTextureCubeArray(probes);

		Texture::createLUT();
		return false;
	};

	processors["invalid"] = [&ipc = mIPC]() {
		ipc.invalid();
		return false;
	};

	while (true)
	{
		std::string cmd;
		mIPC >> cmd;
		auto p = processors.find(cmd);
	
		//Common::Assert(p != processors.end(), "unknonw command: " + cmd);
		if (p == processors.end())
			break;

		if (!p->second())
			break;
	}
}



#endif