
#include "ExampleFramework.h"
#include "Pipeline.h"
#include "SimpleMath.h"
#include "Profile.h"
#pragma comment(lib,"DirectXTK12.lib")

#include "Framework.h"
#include "Renderer.h"
#include "ModelLoader.h"
#include "CameraController.h"
#include "Quad.h"
#include "RenderContext.h"
class Tessellation final : public ExampleFramework
{
	Renderer::PipelineState::Ref pso;
	Renderer::ConstantBuffer::Ptr hardwareCBuffer;
	struct Constants
	{
		DirectX::SimpleMath::Matrix view;
		DirectX::SimpleMath::Matrix proj;
		DirectX::SimpleMath::Vector3 campos;
		float time;
	};

	float curtime = 0;
public:
	Pipeline::Ptr init() override
	{
		auto r = Renderer::getSingleton();
		auto vs = r->compileShaderFromFile("shaders/tessellation.hlsl", "vs", SM_VS);
		auto hs = r->compileShaderFromFile("shaders/tessellation.hlsl", "hs", SM_HS);
		auto ds = r->compileShaderFromFile("shaders/tessellation.hlsl", "ds", SM_DS);
		auto ps = r->compileShaderFromFile("shaders/tessellation.hlsl", "ps", SM_PS);

		hardwareCBuffer = r->createConstantBuffer(sizeof(Constants));

	
		Renderer::RenderState rs(DXGI_FORMAT_R8G8B8A8_UNORM);
		rs.setPrimitiveType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH);

		{
			D3D12_RASTERIZER_DESC desc = {};
			desc.FillMode = D3D12_FILL_MODE_SOLID;
			desc.CullMode = D3D12_CULL_MODE_NONE;
			desc.FrontCounterClockwise = FALSE;
			desc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
			desc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
			desc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
			desc.DepthClipEnable = true;
			desc.MultisampleEnable = FALSE;
			desc.AntialiasedLineEnable = FALSE;
			desc.ForcedSampleCount = 0;
			desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

			rs.setRasterizer(desc);
		}

		std::vector<Renderer::Shader::Ptr> ss = { vs, ps, hs, ds };
		for (auto& shader : ss)
		{
			shader->registerStaticSampler("point_wrap", D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_WRAP);
			shader->registerStaticSampler("point_clamp", D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

			shader->registerStaticSampler("linear_wrap", D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP);
			shader->registerStaticSampler("linear_clamp", D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

			shader->registerStaticSampler("anisotropic_wrap", D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_WRAP);
			shader->registerStaticSampler("anisotropic_clamp", D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
		}

		pso = r->createPipelineState(ss,rs);


		auto tex = r->createTextureFromFile("resources/rustediron/rustediron2_basecolor.png",true);
		auto flow = r->createTextureFromFile("resources/flowmap.png", false);

		tex->createShaderResource();
		flow->createShaderResource();

		pso->setPSResource("albedo", tex->getShaderResource());
		pso->setPSResource("flowmap", flow->getShaderResource());


		auto pipeline = Pipeline::Ptr(new ForwardPipleline);
		//pipeline->addPostProcessPass("tonemapping.hlsl", Renderer::BACK_BUFFER_FORMAT);
		//pipeline->set("tone", true);
		return pipeline;
	}

	void update(float dtime, DirectX::Keyboard& k, DirectX::Mouse& m)
	{
		Constants c;
		using M = DirectX::SimpleMath::Matrix;
		auto size = Renderer::getSingleton()->getSize();
		auto proj = M::CreatePerspectiveFieldOfView(0.75, float(size[0]) / float(size[1]), 0.1f , 100.0f);


		auto view = CameraController::update(dtime, k, m);

		curtime += dtime;

		c.view = view.Transpose();
		c.proj = proj.Transpose();
		c.campos = CameraController::state.pos;
		c.time = curtime;

		hardwareCBuffer->blit(&c, 0, sizeof(Constants));

	}

	void render(Pipeline::Ptr pipeline)
	{
		
		pipeline->addRenderScene([=](auto cmdlist, auto cam, auto, auto){
			auto size = Renderer::getSingleton()->getSize();
			pso->setConstant(Renderer::Shader::ST_DOMAIN, "Constants", hardwareCBuffer);
			pso->setConstant(Renderer::Shader::ST_HULL,"Constants",hardwareCBuffer);
			pso->setConstant(Renderer::Shader::ST_PIXEL, "Constants", hardwareCBuffer);

			cmdlist->setViewport({ 0,0, (float)size[0],(float)size[1],0.0f, 1.0f});
			cmdlist->setScissorRect({ 0,0,(LONG)size[0], (LONG)size[1] });
			cmdlist->setPrimitiveType(D3D_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST);
			cmdlist->setPipelineState(pso);
			cmdlist->drawInstanced(4, 1);
		});
	}

};


REGISTER_EXAMPLE(Tessellation)