#pragma once

#include "ExampleFramework.h"
#include "Pipeline.h"
#include "SimpleMath.h"
#include "Profile.h"
#pragma comment(lib,"DirectXTK12.lib")

#include "RenderCommand.h"
#include "Framework.h"
#include "Renderer.h"

#include <fstream>
class RemoteRender final : public ExampleFramework
{
	using vec3 = DirectX::SimpleMath::Vector3;
	using vec4 = DirectX::SimpleMath::Vector4;
	CommandReceiver mReceiver;
	Renderer::ConstantBuffer::Ptr hardwareCBuffer;
	struct CommonConsts
	{
		Vector3 campos;
		float unuse1;
		Vector3 camdir;
		float unuse2;

		struct 
		{
			Vector3 pos;
			float unuse1;
			Vector3 dir;
			float unuse2;
			Vector4 color;
		}lights[4];

		int numlights;
		Vector3 sundir;
		Vector4 suncolor;
		float deltatime;
		float time;
	}commonCbuffer;
	DirectX::SimpleMath::Matrix viewMatrix;
	Pipeline::Ptr init()
	{
		auto renderer = Renderer::getSingleton();
		hardwareCBuffer = renderer->createConstantBuffer(sizeof(CommonConsts));

		mReceiver.init(false);
		mReceiver.receive();

		auto cam = RenderContext::getSingleton()->getObject<Camera>("main");
		Framework::resize(renderer->getWindow(), cam->viewport.Width, cam->viewport.Height);
		viewMatrix = decltype(viewMatrix)((float*)&cam->view);

		auto pipeline = Pipeline::Ptr(new ForwardPipleline);
		pipeline->addPostProcessPass("tonemapping.hlsl", Renderer::BACK_BUFFER_FORMAT);
		pipeline->set("tone", true);

		return pipeline;
	}
	void update(float dtime, DirectX::Keyboard& k, DirectX::Mouse& m)
	{
		PROFILE("update common cbuffer", {});

		using V = DirectX::SimpleMath::Vector3;
		using V2 = DirectX::SimpleMath::Vector2;

		using M = DirectX::SimpleMath::Matrix;
		using Q = DirectX::SimpleMath::Quaternion;

		static auto lastMS = m.GetState();
		auto ks = k.GetState();
		auto ms = m.GetState();

		V2 pos = { (float)ms.x, (float)ms.y };
		V2 d = pos - V2{(float)lastMS.x, (float)lastMS.y};

		lastMS = ms;

		auto& io = ImGui::GetIO();
		if (io.WantCaptureMouse)
			return;

		auto rc = RenderContext::getSingleton();


		rc->visitObjects<Light>([=](Light::Ptr l){
			switch (l->type)
			{
			case Light::LT_DIR: {commonCbuffer.sundir = l->dir; commonCbuffer.suncolor= l->color; } break;
			case Light::LT_POINT:
			case Light::LT_SPOT:
			default:
				break;
			}
		});

		

		auto cam = rc->getObject<Camera>("main");
		commonCbuffer.deltatime = dtime;
		static float time = 0;
		time += dtime;
		commonCbuffer.time = time;

		static V cp(cam->pos.data());
		V cd(V::UnitX);
		static Q cr = Q::Identity;
		float scale = 100;

		if (ms.leftButton)
		{
			V dir = V::Transform(V::UnitX, cr);
			float pi = 3.14159265358;
			Q tmp =
				Q::CreateFromAxisAngle(dir.Cross(V::UnitZ), -d.y * dtime * 0.1f)
				* Q::CreateFromAxisAngle(V::UnitZ, d.x * dtime * 0.1f)
				;

			cr = cr * tmp;
		}

		cd = V::Transform(V::UnitX, cr);
		commonCbuffer.camdir = { cd.x, cd.y, cd.z };

		V offset = V::Zero;
		if (ks.W)
		{
			offset = cd * scale  * dtime;
		}
		else if (ks.S)
		{
			offset = -cd * scale * dtime;
		}

		else if (ks.A)
		{
			offset = cd.Cross(V::UnitZ) * scale  * dtime;
		}
		else if (ks.D)
		{
			offset = -cd.Cross(V::UnitZ) * scale  * dtime;
		}

		cp += offset;

		commonCbuffer.campos = { cp.x, cp.y, cp.z };


		auto cm = M::CreateFromQuaternion(cr);
		M vpm = M::Identity;
		vpm.m[0][0] = 0;
		vpm.m[1][1] = 0;
		vpm.m[2][2] = 0;

		vpm.m[0][2] = 1;
		vpm.m[1][0] = 1;
		vpm.m[2][1] = 1;

		cm = cm.Transpose() * vpm;
		auto view = M::CreateTranslation(-cp) * cm;
		view = view.Transpose();

		

		viewMatrix = view;
		hardwareCBuffer->blit(&commonCbuffer, 0, sizeof(commonCbuffer));
	}
	void render(Pipeline::Ptr pipeline)
	{
		pipeline->addRenderScene([commonConsts = hardwareCBuffer, viewMatrix = viewMatrix](auto cmdlist, auto cam, UINT, UINT){
			auto renderer = Renderer::getSingleton();
			cmdlist->setViewport(cam->viewport);

			cmdlist->setScissorRect({ 0,0, (LONG)cam->viewport.Width, (LONG)cam->viewport.Height });

			cmdlist->setPrimitiveType();

			RenderContext::getSingleton()->visitObjects<Model>([=](auto model){
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
						cbuffer->setVariable("view", viewMatrix);
						cbuffer->setVariable("proj", cam->proj);
						cbuffer->setVariable("world", model->transform);
						cbuffer->setVariable("nworld", model->normTransform);
						pso->setVSConstant("VSConstant", cbuffer);
						});

					model->visitConstant(Renderer::Shader::ST_PIXEL, i, [&](auto& cbuffer) {
						cbuffer->setVariable("objpos", model->aabb.center);
						cbuffer->setVariable("objradius", model->boundingradius);
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

			});
		});

		pipeline->postprocess("tonemapping.hlsl");

	}
};

REGISTER_EXAMPLE(RemoteRender)