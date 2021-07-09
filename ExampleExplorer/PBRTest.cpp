
#include "ExampleFramework.h"
#include "Pipeline.h"
#include "SimpleMath.h"
#include "Profile.h"
#pragma comment(lib,"DirectXTK12.lib")

#include "Framework.h"
#include "Renderer.h"
#include "ModelLoader.h"
#include "CameraController.h"

#include <fstream>
class PBRTest final : public ExampleFramework
{
public:
	using vec3 = DirectX::SimpleMath::Vector3;
	using vec4 = DirectX::SimpleMath::Vector4;
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

	struct Info
	{
		float nearPlane = 0;
		float farPlane = 0;
	}info;

	ImGuiOverlay::ImGuiWindow* settingWnd;
	struct
	{
		float intensity = 10;
	}settings;



	virtual ~PBRTest()
	{
		settingWnd->destroy();
	}

	Pipeline::Ptr init()
	{
		initScene();
		auto renderer = Renderer::getSingleton();
		hardwareCBuffer = renderer->createConstantBuffer(sizeof(CommonConsts));

		auto pipeline = Pipeline::Ptr(new ForwardPipleline);
		pipeline->addPostProcessPass("tonemapping.hlsl", Renderer::BACK_BUFFER_FORMAT);
		pipeline->set("tone",true);

		settingWnd = ImGuiOverlay::ImGuiObject::root()->createChild<ImGuiOverlay::ImGuiWindow>("settings");
		settingWnd->drawCallback = [=](auto win){
			const float zero = 0;
			ImGui::DragScalar("sun intensity", ImGuiDataType_Float, &settings.intensity,0.01f,&zero,0 );
			return true;
		};

		return pipeline;
	}

	void initScene()
	{
		auto rc = RenderContext::getSingleton();
		auto model = rc->createObject<Model>("pbrtest");
		model->materials.push_back(MaterialMaker("scene_vs.hlsl","pbrtest.hlsl",{
			{"albedo", "rustediron/rustediron2_basecolor.png"},
			{"normal", "rustediron/rustediron2_normal.png"},
			{"roughness", "rustediron/rustediron2_roughness.png"},
			{"metallic", "rustediron/rustediron2_metallic.png"},
		} ));
		model->mesh = QuadMesh()(1);
		memcpy(model->transform.data(), &DirectX::SimpleMath::Matrix::Identity, sizeof(model->transform));
		memcpy(model->normTransform.data(), &DirectX::SimpleMath::Matrix::Identity, sizeof(model->normTransform));
		model->init();
		model->boundingradius = 1;


		//auto model = ModelLoader()("ExampleExplorer/resources/Models/sponza/sponza.fbx");


		//for (auto& m : model->materials)
		//{
		//	std::swap(m->textures["normal"], m->textures["height"]);
		//}


		info.nearPlane = model->boundingradius * 0.01f;
		info.farPlane = model->boundingradius * 2 * 10; ;

	}

	void update(float dtime, DirectX::Keyboard& k, DirectX::Mouse& m)
	{

		using V = DirectX::SimpleMath::Vector3;
		using V2 = DirectX::SimpleMath::Vector2;

		using M = DirectX::SimpleMath::Matrix;
		using Q = DirectX::SimpleMath::Quaternion;

		static auto lastMS = m.GetState();
		auto ks = k.GetState();
		auto ms = m.GetState();

		V2 pos = { (float)ms.x, (float)ms.y };
		V2 d = pos - V2{ (float)lastMS.x, (float)lastMS.y };

		lastMS = ms;


		auto rc = RenderContext::getSingleton();


		rc->visitObjects<Light>([=](Light::Ptr l) {
			switch (l->type)
			{
			case Light::LT_DIR: {
				commonCbuffer.sundir = l->dir; 
				commonCbuffer.suncolor = {
					l->color[0] * settings.intensity,
					l->color[1] * settings.intensity,
					l->color[2] * settings.intensity,
					l->color[3] * settings.intensity
				}; } break;
			case Light::LT_POINT:
			case Light::LT_SPOT:
			default:
				break;
			}
			});

		auto& io = ImGui::GetIO();
		if (!io.WantCaptureMouse)
		{
			commonCbuffer.deltatime = dtime;
			static float time = 0;
			time += dtime;
			commonCbuffer.time = time;


			auto view = CameraController::update(dtime, k, m);

			auto pos = CameraController::state.pos;
			auto dir = CameraController::state.dir;	

			commonCbuffer.camdir = { dir.x, dir.y, dir.z };
			commonCbuffer.campos = { pos.x, pos.y, pos.z };


			auto size = Renderer::getSingleton()->getSize();
			auto proj = M::CreatePerspectiveFieldOfView(0.75,float(size[0]) / float(size[1]), info.nearPlane, info.farPlane);
			view = view.Transpose();
			proj = proj.Transpose();


			auto cam = rc->getObject<Camera>("main");
			memcpy(&cam->view, &view, sizeof(view));
			memcpy(&cam->proj, &proj, sizeof(proj));
		}


		hardwareCBuffer->blit(&commonCbuffer, 0, sizeof(commonCbuffer));
	}
	void render(Pipeline::Ptr pipeline)
	{
		pipeline->addRenderScene([commonConsts = hardwareCBuffer](auto cmdlist, auto cam, UINT, UINT) {
			auto renderer = Renderer::getSingleton();
			cmdlist->setViewport(cam->viewport);

			cmdlist->setScissorRect({ 0,0, (LONG)cam->viewport.Width, (LONG)cam->viewport.Height });

			cmdlist->setPrimitiveType();

			RenderContext::getSingleton()->visitObjects<Model>([=](auto model) {
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
						cbuffer->setVariable("view", cam->view);
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

REGISTER_EXAMPLE(PBRTest)