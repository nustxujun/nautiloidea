#pragma once

#include "ExampleFramework.h"
#include "Pipeline.h"
#include "SimpleMath.h" 

// 光学长度纹理size
constexpr int TRANSMITTANCE_TEXTURE_WIDTH = 256;
constexpr int TRANSMITTANCE_TEXTURE_HEIGHT = 64;

// 内散射(r,mu,mu_s,nu)的size
constexpr int SCATTERING_TEXTURE_R_SIZE = 32;
constexpr int SCATTERING_TEXTURE_MU_SIZE = 128;
constexpr int SCATTERING_TEXTURE_MU_S_SIZE = 32;
constexpr int SCATTERING_TEXTURE_NU_SIZE = 8;

// 内散射积分纹理size,3D纹理
constexpr int SCATTERING_TEXTURE_WIDTH = SCATTERING_TEXTURE_NU_SIZE * SCATTERING_TEXTURE_MU_S_SIZE;
constexpr int SCATTERING_TEXTURE_HEIGHT = SCATTERING_TEXTURE_MU_SIZE;
constexpr int SCATTERING_TEXTURE_DEPTH = SCATTERING_TEXTURE_R_SIZE;

static const int IRRADIANCE_TEXTURE_WIDTH = 64;
static const int IRRADIANCE_TEXTURE_HEIGHT = 16;

using namespace DirectX;

class AtmosphericScatteringExample : public ExampleFramework
{
    enum
    {
        TRANSMITTANCE,
        SINGLESCATTERING,
        DIRECT_IRRADIANCE,

        FINAL,
        NUM
    };


    ForwardPipleline pipeline;
    Quad mQuad;
    Renderer::ConstantBuffer::Ptr mCommonConsts;
    Renderer::ConstantBuffer::Ptr mFinalConsts;
    std::array<Renderer::PipelineState::Ref, NUM> mPSOs;

    ResourceHandle::Ptr mTransmittance;
    ResourceHandle::Ptr mScattering;
    ResourceHandle::Ptr mSingleMieScattering;
    ResourceHandle::Ptr mIrradiance;

    size_t mNumScattering = 8;
    bool mRecompute = true;

    struct DensityProfileLayer
    {
        float width = 0;
        float exp_term = 0;
        float exp_scale = 0;
        float linear_term = 0;

        float constant_term = 0;
        float3 unused = {};
    };

    struct
    {
        struct
        {
            struct DensityProfile
            {
                DensityProfileLayer layers[2];
            };

            float3 solar_irradiance;
            float sun_angular_radius;

            float3 rayleigh_scattering;
            float bottom_radius;

            float3 mie_scattering;
            float top_radius;

            float3 mie_extinction;
            float mie_phase_function_g;

            float3 absorption_extinction;
            float mu_s_min;

            float3 ground_albedo;
            float unused;

            DensityProfile rayleigh_density;
            DensityProfile mie_density;

            DensityProfile absorption_density;
        } params;

    }mAtmosphereParams;

	ImGuiOverlay::ImGuiObject* mSettings;

public:
    AtmosphericScatteringExample()
    {
		initResource();
        initAtmosphere();
    }
    void update(float dtime)
    {}

    void render()
    {
        pipeline.postProcess([=](RenderGraph& graph, auto rt, auto ds){
            
			if (mRecompute) {
				mRecompute = false;
				auto delta_rayleigh_scattering = ResourceHandle::create(
					Renderer::VT_UNORDEREDACCESS,
					SCATTERING_TEXTURE_WIDTH,
					SCATTERING_TEXTURE_HEIGHT,
					SCATTERING_TEXTURE_DEPTH,
					DXGI_FORMAT_R32G32B32A32_FLOAT);

				auto delta_mie_scattering = ResourceHandle::create(
					Renderer::VT_UNORDEREDACCESS,
					SCATTERING_TEXTURE_WIDTH,
					SCATTERING_TEXTURE_HEIGHT,
					SCATTERING_TEXTURE_DEPTH,
					DXGI_FORMAT_R32G32B32A32_FLOAT);

				auto delta_irradiance = ResourceHandle::create(
					Renderer::VT_UNORDEREDACCESS,
					IRRADIANCE_TEXTURE_WIDTH,
					IRRADIANCE_TEXTURE_HEIGHT,
					DXGI_FORMAT_R32G32B32A32_FLOAT);

				graph.addPass("prec_trans", [this](RenderGraph::Builder& b)mutable
					{
						b.access(mTransmittance);
						return [this](Renderer::CommandList::Ref cmdlist) {
							auto pso = mPSOs[TRANSMITTANCE];
							pso->setCSResource("transmittanceTex_o", mTransmittance->getView()->getShaderResource());
							pso->setCSConstant("AtomsphereConstants", mCommonConsts);
							cmdlist->setPipelineState(pso);
							cmdlist->dispatch(TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_HEIGHT, 1);
					};
				});

				graph.addPass("prec_ss", [=](RenderGraph::Builder& b) {
					b.access(mScattering);
					b.access(mSingleMieScattering);
					b.read(mTransmittance);
					return [=](Renderer::CommandList::Ref cmdlist) {
						auto pso = mPSOs[SINGLESCATTERING];
						pso->setCSResource("transmittanceTex", mTransmittance->getView()->getShaderResource());
						pso->setCSResource("scatteringTex_o", mScattering->getView()->getUnorderedAccess());
						pso->setCSResource("single_mie_scatteringTex_o", mSingleMieScattering->getView()->getUnorderedAccess());
						pso->setCSResource("rayleigh_scatteringTex_o", delta_rayleigh_scattering->getView()->getUnorderedAccess());
						pso->setCSResource("mie_scatteringTex_o", delta_mie_scattering->getView()->getUnorderedAccess());


						pso->setCSConstant("AtomsphereConstants", mCommonConsts);
						cmdlist->setPipelineState(pso);
						cmdlist->dispatch(SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT, SCATTERING_TEXTURE_DEPTH);
					};
				});

				graph.addPass("prec_di", [=](RenderGraph::Builder& b) {
					b.access(mScattering);
					b.access(mSingleMieScattering);
					b.read(mTransmittance);
					return [=](Renderer::CommandList::Ref cmdlist) {
						auto pso = mPSOs[DIRECT_IRRADIANCE];
						pso->setCSResource("transmittanceTex", mTransmittance->getView()->getShaderResource());
						pso->setCSResource("deltaIrradianceTex_o", delta_irradiance->getView()->getUnorderedAccess());
						pso->setCSResource("irradianceTex_o", mIrradiance->getView()->getUnorderedAccess());

						pso->setCSConstant("AtomsphereConstants", mCommonConsts);
						cmdlist->setPipelineState(pso);
						cmdlist->dispatch(IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT, 1);
					};
				});

				for (auto i = 2; i < mNumScattering; ++i)
				{

				}
			}

			graph.addPass("atmosphere", [this, rt](RenderGraph::Builder& b) {
				b.write(rt, RenderGraph::Builder::IT_NONE);
				return [this, rt](Renderer::CommandList::Ref cmdlist)
				{
					mQuad.setResource("transmittanceTex", mTransmittance->getView()->getShaderResource());
					mQuad.setResource("scatteringTex", mScattering->getView()->getShaderResource());
					mQuad.setResource("single_mie_scatteringTex", mSingleMieScattering->getView()->getShaderResource());
					mQuad.setResource("irradianceTex", mIrradiance->getView()->getShaderResource());

					mQuad.setConstants("AtomsphereConstants", mCommonConsts);
					mQuad.setConstants("AtmosphericScatteringFinal", mFinalConsts);

					cmdlist->setRenderTarget(rt->getView());
					mQuad.fitToScreen();
					mQuad.draw(cmdlist);
				};
				});
			
			return rt;
        });
        pipeline.execute();
    }

    void initAtmosphere()
    {

		constexpr double kLambdaR = 680.0;
		constexpr double kLambdaG = 550.0;
		constexpr double kLambdaB = 440.0;

		constexpr double mie_g_coefficient = 0.8;
		constexpr bool use_half_precision_ = true;
		constexpr bool use_constant_solar_spectrum_ = false;
		constexpr bool use_ozone_ = true;
		constexpr bool use_rayleigh_scattering = true;
		constexpr bool use_mie_scattering = true;

		constexpr double kPi = 3.1415926;
		constexpr double kSunAngularRadius = 0.00935 / 2.0;
		constexpr double kSunSolidAngle = kPi * kSunAngularRadius * kSunAngularRadius;
		constexpr double kLengthUnitInMeters = 1000.0;

		constexpr int kLambdaMin = 360;
		constexpr int kLambdaMax = 830;
		constexpr double kSolarIrradiance[48] = {
			1.11776, 1.14259, 1.01249, 1.14716, 1.72765, 1.73054, 1.68870, 1.61253,
			1.91198, 2.03474, 2.02042, 2.02212, 1.93377, 1.95809, 1.91686, 1.82980,
			1.86850, 1.89310, 1.85149, 1.85040, 1.83410, 1.83450, 1.81470, 1.78158, 1.7533,
			1.69650, 1.68194, 1.64654, 1.60480, 1.52143, 1.55622, 1.51130, 1.47400, 1.4482,
			1.41018, 1.36775, 1.34188, 1.31429, 1.28303, 1.26758, 1.23670, 1.20820,
			1.18737, 1.14683, 1.12362, 1.10580, 1.07124, 1.04992
		};

		// http://www.iup.uni-bremen.de/gruppen/molspec/databases
		// /referencespectra/o3spectra2011/index.html
		constexpr double kOzoneCrossSection[48] = {
			1.18e-27, 2.182e-28, 2.818e-28, 6.636e-28, 1.527e-27, 2.763e-27, 5.52e-27,
			8.451e-27, 1.582e-26, 2.316e-26, 3.669e-26, 4.924e-26, 7.752e-26, 9.016e-26,
			1.48e-25, 1.602e-25, 2.139e-25, 2.755e-25, 3.091e-25, 3.5e-25, 4.266e-25,
			4.672e-25, 4.398e-25, 4.701e-25, 5.019e-25, 4.305e-25, 3.74e-25, 3.215e-25,
			2.662e-25, 2.238e-25, 1.852e-25, 1.473e-25, 1.209e-25, 9.423e-26, 7.455e-26,
			6.566e-26, 5.105e-26, 4.15e-26, 4.228e-26, 3.237e-26, 2.451e-26, 2.801e-26,
			2.534e-26, 1.624e-26, 1.465e-26, 2.078e-26, 1.383e-26, 7.105e-27
		};
		// https://en.wikipedia.org/wiki/Dobson_unit, in molecules.m^-2.
		constexpr double kDobsonUnit = 2.687e20;
		constexpr double kMaxOzoneNumberDensity = 300.0 * kDobsonUnit / 15000.0;
		constexpr double kConstantSolarIrradiance = 1.5;
		constexpr double kBottomRadius = 6360000.0;
		constexpr double kTopRadius = 6420000.0;
		constexpr double kRayleigh = 1.24062e-6;
		constexpr double kRayleighScaleHeight = 8000.0;
		constexpr double kMieScaleHeight = 1200.0;
		constexpr double kMieAngstromAlpha = 0.0;
		constexpr double kMieAngstromBeta = 5.328e-3;
		constexpr double kMieSingleScatteringAlbedo = 0.9;
		double kMiePhaseFunctionG = mie_g_coefficient;
		constexpr double kGroundAlbedo = 0.1;
		const double max_sun_zenith_angle =
			(use_half_precision_ ? 102.0 : 120.0) / 180.0 * kPi;

		DensityProfileLayer rayleigh_layer = { 0.0, 1.0 * use_rayleigh_scattering,
			-1.0 / kRayleighScaleHeight * use_rayleigh_scattering,
			0.0, 0.0 };
		DensityProfileLayer mie_layer = { 0.0, 1.0 * use_mie_scattering,
			-1.0 / kMieScaleHeight * use_mie_scattering, 0.0, 0.0 };
		std::vector<DensityProfileLayer> ozone_density;
		ozone_density.push_back({ 25000.0, 0.0, 0.0, 1.0 / 15000.0, -2.0 / 3.0 });
		ozone_density.push_back({ 0.0, 0.0, 0.0, -1.0 / 15000.0, 8.0 / 3.0 });



		std::vector<double> wavelengths;           
		std::vector<double> solar_irradiance;      
		std::vector<double> rayleigh_scattering;    
		std::vector<double> mie_scattering;       
		std::vector<double> mie_extinction;         
		std::vector<double> absorption_extinction;  
		std::vector<double> ground_albedo;          
		for (int l = kLambdaMin; l <= kLambdaMax; l += 10) {
			double lambda = static_cast<double>(l) * 1e-3;
			double mie =
				kMieAngstromBeta / kMieScaleHeight * pow(lambda, -kMieAngstromAlpha);
			wavelengths.push_back(l);
			if (use_constant_solar_spectrum_) {
				solar_irradiance.push_back(kConstantSolarIrradiance);
			}
			else {
				solar_irradiance.push_back(kSolarIrradiance[(l - kLambdaMin) / 10]);
			}
			rayleigh_scattering.push_back(kRayleigh * pow(lambda, -4));
			mie_scattering.push_back(mie * kMieSingleScatteringAlbedo);
			mie_extinction.push_back(mie);
			absorption_extinction.push_back(
				use_ozone_ * kMaxOzoneNumberDensity * kOzoneCrossSection[(l - kLambdaMin) / 10]);
			ground_albedo.push_back(kGroundAlbedo);
		}

		auto interpolate = [&](auto& function, double wavelength)
		{
			if (wavelength < wavelengths[0]) {
				return function[0];
			}
			for (unsigned int i = 0; i < wavelengths.size() - 1; ++i) {
				if (wavelength < wavelengths[i + 1]) {
					double u =
						(wavelength - wavelengths[i]) / (wavelengths[i + 1] - wavelengths[i]);
					return
						function[i] * (1.0 - u) + function[i + 1] * u;
				}
			}
			return function[function.size() - 1];
		};

		float3 lambdas{ kLambdaR, kLambdaG, kLambdaB };

		auto cal_val = [&](auto& v, double scale) {
			float r = interpolate(v, lambdas[0]) * scale;
			float g = interpolate(v, lambdas[1]) * scale;
			float b = interpolate(v, lambdas[2]) * scale;
			return float3{ r,g,b };
		};


		mAtmosphereParams = {
			{
				cal_val(solar_irradiance,  1.0),
				kSunAngularRadius,

				cal_val(rayleigh_scattering,  kLengthUnitInMeters),
				kBottomRadius,

				cal_val(mie_scattering,  kLengthUnitInMeters),
				kTopRadius,

				cal_val(mie_extinction,  kLengthUnitInMeters),
				(float)kMiePhaseFunctionG,

				cal_val(absorption_extinction,kLengthUnitInMeters),
				max_sun_zenith_angle,

				cal_val(ground_albedo, 1.0),
				0,

				{{{}, rayleigh_layer}},
				{{{}, mie_layer}},
				{{ozone_density[0], ozone_density[1]}},
			},
		};

		mCommonConsts->blit(&mAtmosphereParams, 0, sizeof(mAtmosphereParams));

		{
			mFinalConsts->setVariable("camera", Vector3{ 0, 100000000.0f ,0 });
			mFinalConsts->setVariable("earth_radius", float(6360000.0f));
			mFinalConsts->setVariable("view_dir", Vector3{ 0,-1,0 });
			mFinalConsts->setVariable("sun_tan", std::tan(float(kSunAngularRadius)));
			mFinalConsts->setVariable("sun_cos", std::cos(float(kSunAngularRadius)));
			mFinalConsts->setVariable("exposure", float(10.0f));

			float sun_azimuth_angle_radians_ = 2.9;
			float sun_zenith_angle_radians_ = 1.3;

			mFinalConsts->setVariable("sun_dir",
				Vector3{ cos(sun_azimuth_angle_radians_) * sin(sun_zenith_angle_radians_),
						sin(sun_azimuth_angle_radians_) * sin(sun_zenith_angle_radians_),
						cos(sun_zenith_angle_radians_) });


		}
    }

	void initResource()
	{
		auto const FORMAT = DXGI_FORMAT_R32G32B32A32_FLOAT;


		auto renderer = Renderer::getSingleton();
		std::string shader = "shaders/atmospheric_scattering.hlsl";
		{
			auto cs = renderer->compileShaderFromFile(shader, "precomputeTransmittanceToAtmosphereTop", SM_CS);
			mPSOs[TRANSMITTANCE] = renderer->createComputePipelineState(cs);
			mCommonConsts = mPSOs[TRANSMITTANCE]->createConstantBuffer(Renderer::Shader::ST_COMPUTE, "AtomsphereConstants");
		}

		{
			auto cs = renderer->compileShaderFromFile(shader, "precomputeSingleScatteringTexture", SM_CS);
			cs->registerStaticSampler("pointSampler", D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

			mPSOs[SINGLESCATTERING] = renderer->createComputePipelineState({ cs });
		}

		{
			auto cs = renderer->compileShaderFromFile(shader, "precomputeDirectIrradiance", SM_CS);
			cs->registerStaticSampler("pointSampler", D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

			mPSOs[DIRECT_IRRADIANCE] = renderer->createComputePipelineState({ cs });
		}

		{
			auto ps = renderer->compileShaderFromFile(shader, "atmospheric_scattering", SM_PS);
			mQuad.init(ps, Renderer::FRAME_BUFFER_FORMAT);
			mFinalConsts = mQuad.getPipelineState()->createConstantBuffer(Renderer::Shader::ST_PIXEL, "AtmosphericScatteringFinal");


		}
		auto size = renderer->getSize();
		mTransmittance = ResourceHandle::create(Renderer::VT_UNORDEREDACCESS, TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_HEIGHT, FORMAT);
		mScattering = ResourceHandle::create(Renderer::VT_UNORDEREDACCESS, SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT, SCATTERING_TEXTURE_DEPTH, FORMAT);
		mSingleMieScattering = ResourceHandle::create(Renderer::VT_UNORDEREDACCESS, SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT, SCATTERING_TEXTURE_DEPTH, FORMAT);
		mIrradiance = ResourceHandle::create(Renderer::VT_UNORDEREDACCESS, IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT, FORMAT);

		mSettings = ImGuiOverlay::ImGuiObject::root()->createChild<ImGuiOverlay::ImGuiWindow>("atmosphere settings");
		mSettings->drawCallback = [&](auto ui) {

			return true;
		};
	}
} ;


REGISTER_EXAMPLE(AtmosphericScatteringExample)