#ifdef NO_UE4
#pragma once


#include "Engine/Framework.h"
#include "Engine/Pipeline.h"
#include "Engine/RenderContext.h"
#include "Engine/Renderer.h"
#include "Profile.h"

#include "RenderCommand.h"
#include <array>
#include <chrono>
class DefaultFrame :public RenderContext, public Framework
{
public:
	CommandReceiver receiver;
	std::shared_ptr<DefaultPipeline> pipeline;
	Renderer::ConstantBuffer::Ptr commonConsts;
	float deltaTime = 0;
	float time = 0;
	std::chrono::high_resolution_clock::time_point timepoint;
public:
	void init(bool runwithipc = false)
	{
		Framework::initialize();
		pipeline = decltype(pipeline)(new DefaultPipeline);

		if (runwithipc)
		{
			receiver.init(false);
			receiver.receive();
			commonConsts = mRenderList[0]->materials[0]->pipelineState->createConstantBuffer(Renderer::Shader::ST_PIXEL,"CommonConstants");
		}
		auto cam = getObject<Camera>("main");

		Framework::resize(cam->viewport.Width, cam->viewport.Height);
	}

	void renderScreen()
	{
	}

	void updateConsts()
	{
		if (!commonConsts)
			return;
	
		Vector3 sundir;
		Vector4 camdir, campos;
		Color suncolor;
		for (auto& l : mLights)
		{
			switch (l->type)
			{
			case Light::LT_DIR:{sundir = l->dir; suncolor = l->color;} break;
			case Light::LT_POINT:
			case Light::LT_SPOT:
			default:
				break;
			}
		}

		auto cam = getObject<Camera>("main");
		camdir = { cam->dir[0],cam->dir[1],cam->dir[2],0 };
		campos = { cam->pos[0],cam->pos[1],cam->pos[2],0 };


		commonConsts->setVariable("campos", &campos);
		commonConsts->setVariable("camdir", &camdir);
		int numlights = 0;
		commonConsts->setVariable("numlights", &numlights);
		commonConsts->setVariable("sundir", &sundir);
		commonConsts->setVariable("suncolor", &suncolor);
		commonConsts->setVariable("deltatime", &deltaTime);
		commonConsts->setVariable("time", &time);
		
	}

	void updateImpl()
	{
		static auto lastTime = std::chrono::high_resolution_clock::now();
		auto cur = std::chrono::high_resolution_clock::now();
		deltaTime = (cur - lastTime).count() / 1000000000.0f;
		time += deltaTime;
		{
			lastTime = cur;
			static float history = 60;
			history = history * 0.99f +  (1.0f / deltaTime) * 0.01f;

			std::stringstream ss;
			ss.precision(4);
			ss << history << "(" <<  1000.0f / history<< "ms)";
			::SetWindowTextA(Renderer::getSingleton()->getWindow(), ss.str().c_str());
		}

		updateConsts();
		pipeline->update();

	}

	void renderScene(Renderer::CommandList::Ref cmdlist, Camera::Ptr cam, UINT, UINT)
	{
		auto renderer = Renderer::getSingleton();
		cmdlist->setViewport(cam->viewport);

		cmdlist->setScissorRect({ 0,0, (LONG)cam->viewport.Width, (LONG)cam->viewport.Height });

		cmdlist->setPrimitiveType();

		for (auto& model : mRenderList)
		{


			UINT numMaterials = (UINT)model->materials.size();
			std::vector<std::vector<Mesh::SubMesh*>> subs(numMaterials);

			auto mesh = model->mesh;
			for (auto& sm : mesh->submeshes)
			{
				subs[sm.materialIndex].push_back(&sm);
			}

			for (UINT i = 0; i < numMaterials; ++i)
			{
				auto& material = model->materials[i];
				material->applyTextures();
				auto& pso = material->pipelineState;

				model->visitConstant(Renderer::Shader::ST_VERTEX, i, [&](auto& cbuffer) {
					cbuffer->setVariable("view", &cam->view);
					cbuffer->setVariable("proj", &cam->proj);
					cbuffer->setVariable("world", &model->transform);
					cbuffer->setVariable("nworld", &model->normTransform);
					pso->setVSConstant("VSConstant", cbuffer);
					});

				model->visitConstant(Renderer::Shader::ST_PIXEL, i, [&](auto& cbuffer) {
					cbuffer->setVariable("objpos", &model->aabb.center);
					cbuffer->setVariable("objradius", &model->boundingradius);
					pso->setPSConstant("PSConstant", cbuffer);
					});


				if (commonConsts)
					pso->setPSConstant("CommonConstants", commonConsts);

				cmdlist->setPipelineState(pso);

				for (auto& sm : subs[i])
				{
					cmdlist->setVertexBuffer(mesh->vertices);
					cmdlist->setIndexBuffer(mesh->indices);
					cmdlist->drawIndexedInstanced(sm->numIndices, 1U, sm->startIndex);
				}
			}
		}

		
	}

};


#endif