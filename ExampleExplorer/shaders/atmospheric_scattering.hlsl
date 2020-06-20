#include "common.hlsl"

// 光学长度纹理size
static const int TRANSMITTANCE_TEXTURE_WIDTH = 256;
static const int TRANSMITTANCE_TEXTURE_HEIGHT = 64;

// 内散射(r,mu,mu_s,nu)的size
static const int SCATTERING_TEXTURE_R_SIZE = 32;
static const int SCATTERING_TEXTURE_MU_SIZE = 128;
static const int SCATTERING_TEXTURE_MU_S_SIZE = 32;
static const int SCATTERING_TEXTURE_NU_SIZE = 8;

// 内散射积分纹理size,3D纹理
static const int SCATTERING_TEXTURE_WIDTH = SCATTERING_TEXTURE_NU_SIZE * SCATTERING_TEXTURE_MU_S_SIZE;
static const int SCATTERING_TEXTURE_HEIGHT = SCATTERING_TEXTURE_MU_SIZE;
static const int SCATTERING_TEXTURE_DEPTH = SCATTERING_TEXTURE_R_SIZE;

// 辐照度纹理size
static const int IRRADIANCE_TEXTURE_WIDTH = 64;
static const int IRRADIANCE_TEXTURE_HEIGHT = 16;

static const float2 IRRADIANCE_TEXTURE_SIZE = float2(IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT);

static const float PI = 3.14159265;
static const float3  kGroundAlbedo = float3(0.0, 0.0, 0.04);

sampler pointSampler: s0;
sampler linearSampler: s1;

Texture2D transmittance_texture;
Texture3D scattering_texture;
Texture3D delta_single_rayleigh_scattering_texture;
Texture3D single_mie_scattering_texture;
Texture3D delta_single_mie_scattering_texture;

Texture2D irradiance_texture;
Texture3D delta_multiple_scattering_texture;
Texture2D delta_irradianceTex;
Texture3D delta_scattering_density_texture;

RWTexture2D<float4> transmittanceTex_o;
RWTexture2D<float4> deltaIrradianceTex_o;
RWTexture2D<float4> irradianceTex_o;

RWTexture3D<float4> rayleigh_scatteringTex_o;
RWTexture3D<float4> mie_scatteringTex_o;
RWTexture3D<float4> scatteringTex_o;
RWTexture3D<float4> single_mie_scatteringTex_o;
RWTexture3D<float4> scattering_densityTex_o;
RWTexture3D<float4> multiple_scatteringTex_o;
struct DensityProfileLayer
{
	float width;
	float exp_term;
	float exp_scale;
	float linear_term;
	float constant_term;
	float3 unused;
};

struct DensityProfile
{
	DensityProfileLayer layers[2];
};

	   
struct AtmosphereParameters
{
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
};

cbuffer AtomsphereConstants
{
	AtmosphereParameters parameters;
};


float clampCosine(float mu){
	return(clamp(mu, -1.0f, 1.0f));
}

float clampRadius(AtmosphereParameters params, float r)
{
	return clamp(r, params.bottom_radius, params.top_radius);
}

float safeSqrt(float a) {
	return(sqrt(max(a, 0.0f)));
}

float getLayerDensity(DensityProfileLayer layer, float altitude)
{
	float density = layer.exp_term * exp( layer.exp_scale * altitude ) + layer.linear_term * altitude + layer.constant_term;
	return clamp(density, 0, 1.0f);
}

float getProfileDensity(DensityProfile profile, float altitude)
{
	return altitude < profile.layers[0].width ? getLayerDensity(profile.layers[0], altitude) : getLayerDensity(profile.layers[1], altitude);
}

float distanceToAtmosphereTop(AtmosphereParameters params, float r, float mu)
{
	float d = r * r * (mu * mu - 1.0f) + params.top_radius * params.top_radius;
	return max(-r * mu + sqrt( max(d, 0)), 0);
}

float calculateOpticalDepthToAtmosphereTop(AtmosphereParameters params, DensityProfile profile, float r, float mu)
{
	const int SAMPLE_COUNT = 500;
	float dx = distanceToAtmosphereTop(params, r, mu) / float(SAMPLE_COUNT);
	float result = 0;
	for (int i = 0; i <= SAMPLE_COUNT; ++i)
	{
		float d_i = float(i) * dx;
		float r_i = sqrt(d_i * d_i + 2.0f * r * mu * d_i + r * r );
		float y_i = getProfileDensity(profile, r_i - params.bottom_radius);
		float weight_i = i == 0 || i == SAMPLE_COUNT ? 0.5f : 1.0f; 
		result += y_i * weight_i * dx;
	}
	return result;
}

float3 calculateTransmittanceToAtmosphereTop(AtmosphereParameters params, float r, float mu)
{
	float3 ret = -(
		params.rayleigh_scattering *
		calculateOpticalDepthToAtmosphereTop(
			params, params.rayleigh_density, r, mu) +
		params.mie_extinction *
		calculateOpticalDepthToAtmosphereTop(
			params, params.mie_density, r, mu) +
		params.absorption_extinction *
		calculateOpticalDepthToAtmosphereTop(
			params, params.absorption_density, r, mu));
	ret = exp(ret);
	return ret;
}

float getUnitRangeFromTextureCoord(float u, float size)
{
	return (( u - 0.5f / size) / (1.0f - 1.0f / size));
}

void getRMUFromTransmittanceTextureCoord(AtmosphereParameters params,float2 uv, out float r,out float mu)
{
	float x_mu = getUnitRangeFromTextureCoord(uv.x, TRANSMITTANCE_TEXTURE_WIDTH);
	float x_r = getUnitRangeFromTextureCoord(uv.y, TRANSMITTANCE_TEXTURE_HEIGHT);

	float bb = params.bottom_radius * params.bottom_radius;

	float H = sqrt(params.top_radius * params.top_radius - bb);
	float rho = H * x_r;
	r = sqrt(rho * rho + bb );
	float d_min = params.top_radius - r;
	float d_max = rho + H;
	float d = d_min + x_mu * (d_max - d_min);
	mu = d == 0 ? 1.0f : (H * H - rho * rho - d * d) / (2.0f * r * d);
	mu = clampCosine(mu);
}

float3 calculateTransmittanceToAtmosphereTopTexture(AtmosphereParameters params, float2 uv)
{
	float r, mu;
	getRMUFromTransmittanceTextureCoord(params, uv , r, mu);
	return calculateTransmittanceToAtmosphereTop(params, r, mu);
}


//float4 precomputeTransmittanceToAtmosphereTop(QuadInput input) :SV_Target
//{
//	return float4(calculateTransmittanceToAtmosphereTopTexture(parameters, input.uv), 1);
//}


[numthreads(1, 1, 1)]
void precomputeTransmittanceToAtmosphereTop(uint3 globalIdx: SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx : SV_GroupID)
{
	float2 uv = (float2(globalIdx.xy) + 0.5f ) / float2(TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_HEIGHT);
	float4 ret = float4(calculateTransmittanceToAtmosphereTopTexture(parameters, uv), 1);

	transmittanceTex_o[globalIdx.xy] = ret;
}

//=============================================================================================================

float getTextureCoordFromUnitRange(float x, float size) {
	return(0.5f / size + x * (1.0f - 1.0f / size));
}

float distanceToAtmosphereBottom( AtmosphereParameters params, float r,float mu ){
	float d = r * r * (mu * mu - 1.0f) + params.bottom_radius * params.bottom_radius;
	return max(-r * mu - sqrt( max(d, 0)), 0);
}

float distanceToNearestAtmosphereBoundary(AtmosphereParameters params, float r, float mu, bool intersect_ground)
{
	if (intersect_ground)
		return distanceToAtmosphereBottom(params, r,mu);
	else
		return distanceToAtmosphereTop(params, r, mu);
}

float2 getTransmittanceTextureCoordFromRMu(AtmosphereParameters params, float  r, float mu) {
	float H = sqrt(params.top_radius * params.top_radius -
		params.bottom_radius * params.bottom_radius);
	/* 过视点的地表切线的切点到视点的距离 */
	float rho = sqrt( max(r * r - params.bottom_radius * params.bottom_radius, 0));
	/* d为视点沿视线方向与大气层外层的交点的距离,后两个是d的上界、下界*/
	float	d = distanceToAtmosphereTop(params, r, mu);
	float	d_min = params.top_radius - r;// 下界是视点到大气层的垂直距离
	float	d_max = rho + H; // 上界的情况是视线刚好与地表相切
	float	x_mu = (d - d_min) / (d_max - d_min); // 将mu映射到[0,1]
	float	x_r = rho / H;
	return float2(getTextureCoordFromUnitRange(x_mu, TRANSMITTANCE_TEXTURE_WIDTH),
		getTextureCoordFromUnitRange(x_r, TRANSMITTANCE_TEXTURE_HEIGHT));
}

float3 getTransmittanceToAtmosphereTop(AtmosphereParameters params, Texture2D transmittanceTexture, float r, float mu)
{
	float2 uv = getTransmittanceTextureCoordFromRMu(params, r, mu);
	return float3(transmittanceTexture.SampleLevel(pointSampler, uv, 0).rgb);
}

float3 getTransmittance(
	AtmosphereParameters params, 
	Texture2D transmittanceTexture, 
	float r,
	float mu, 
	float d,
	bool intersect_ground)
{
	float r_d = clampRadius(params, sqrt( d * d + 2.0f * r * mu * d + r * r ));
	float mu_d = clampCosine((r * mu + d) / r_d);

	if (intersect_ground)
	{
		return min(
			getTransmittanceToAtmosphereTop( params, transmittanceTexture, r_d, -mu_d) / 
			getTransmittanceToAtmosphereTop( params, transmittanceTexture, r, -mu),
			1.0f
		);
	}
	else
	{
		return min(
			getTransmittanceToAtmosphereTop( params, transmittanceTexture, r, mu) / 
			getTransmittanceToAtmosphereTop( params, transmittanceTexture, r_d, mu_d),
			1.0f
		);	
	}
}

float3 getTransmittanceToSun(AtmosphereParameters params, Texture2D transmittanceTexture, float r, float mu_s)
{
	float sin_theta_h = params.bottom_radius / r;
	float cos_theta_h = -sqrt(max(1.0f - sin_theta_h * sin_theta_h, 0.0f));

	return getTransmittanceToAtmosphereTop(params, transmittanceTexture , r, mu_s) * 
		smoothstep(-sin_theta_h * params.sun_angular_radius , sin_theta_h * params.sun_angular_radius, mu_s - cos_theta_h);
}


void calculateSingleScatteringIntegrand(
	AtmosphereParameters params, 
	Texture2D transmittanceTexture, 
	float r, 
	float mu,
	float mu_s,
	float nu, 
	float d,
	bool intersect_ground,
	out float3 rayleigh, 
	out float3 mie )
{
	float r_d = clampRadius(params, sqrt( d * d + 2.0f * r * mu * d + r * r ));
	float mu_s_d = clampCosine( (r * mu_s + d * nu) / r_d );
	float3 transmittance = 
		getTransmittance(params, transmittanceTexture, r, mu, d, intersect_ground) * 
		getTransmittanceToSun(params, transmittanceTexture, r_d, mu_s_d);

	rayleigh = transmittance * getProfileDensity(params.rayleigh_density, r_d - params.bottom_radius);
	mie = transmittance * getProfileDensity(params.mie_density, r_d - params.bottom_radius);
}

void calculateSingleScattering(
	AtmosphereParameters params, 
	Texture2D transmittanceTexture, 
	float r, 
	float mu, 
	float mu_s,
	float nu,
	bool ray_r_mu_intersects_ground,
	out float3 rayleigh, 
	out float3 mie
	)
{
	const int SAMPLE_COUNT = 50;
	float dx = distanceToNearestAtmosphereBoundary(params, r, mu, ray_r_mu_intersects_ground) / float(SAMPLE_COUNT);
	float3 rayleigh_sum = 0;
	float3 mie_sum = 0;

	for (int i = 0; i <= SAMPLE_COUNT; ++i)
	{
		float d_i = float(i) * dx;
		float3 rayleigh_i;
		float3 mie_i;

		calculateSingleScatteringIntegrand(
			params, transmittanceTexture, r, mu, mu_s, nu, d_i, ray_r_mu_intersects_ground, rayleigh_i, mie_i);

		float weight_i = (i == 0 || i == SAMPLE_COUNT) ? 0.5f: 1.0f;
		rayleigh_sum += rayleigh_i * weight_i;
		mie_sum += mie_i * weight_i;	
	}

	rayleigh = rayleigh_sum * dx * params.solar_irradiance * params.rayleigh_scattering;
	mie = mie_sum * dx * params.solar_irradiance * params.mie_scattering;
}

void getRMuMuSNuFromScatteringTextureUvwz(
	AtmosphereParameters params, 
	float4 uvwz,
	out float r,
	out float mu,
	out float mu_s,
	out float nu,
	out bool intersect_ground)
{
	float H = sqrt(params.top_radius * params.top_radius - params.bottom_radius * params.bottom_radius);
	float rho = H * getUnitRangeFromTextureCoord(uvwz.w, SCATTERING_TEXTURE_R_SIZE);
	r = sqrt(rho * rho + params.bottom_radius * params.bottom_radius);

	if (uvwz.z < 0.5)
	{
		float d_min = r - params.bottom_radius;
		float d_max = rho;
		float d = d_min + (d_max - d_min) * getUnitRangeFromTextureCoord(
			1.0f - 2.0f * uvwz.z , SCATTERING_TEXTURE_MU_SIZE / 2);
		mu = d == 0 ? -1.0f: clampCosine( -(rho * rho + d * d) / (2.0f * r * d) );

		intersect_ground = true;
	}
	else
	{
		float d_min = params.top_radius - r;
		float d_max = rho + H;
		float d = d_min + (d_max - d_min) * getUnitRangeFromTextureCoord(
			2.0f * uvwz.z - 1.0f, SCATTERING_TEXTURE_MU_SIZE / 2);

		mu = d == 0 ? 1.0f : clampCosine( (H * H - rho * rho - d * d) / (2.0f * r * d) );

		intersect_ground = false;
	}

	float x_mu_s = getUnitRangeFromTextureCoord(uvwz.y, SCATTERING_TEXTURE_MU_SIZE);

	float	d_min = params.top_radius - params.bottom_radius;
	float	d_max = H;
	float	A = -2.0f * params.mu_s_min * params.bottom_radius / (d_max - d_min);
	float	a = (A - x_mu_s * A) / (1.0f + x_mu_s * A);
	float	d = d_min + min(a, A) * (d_max - d_min);
	mu_s = d == 0? 1.0f : 
		clampCosine((H * H - d * d) / (2.0 * params.bottom_radius * d));
	nu = clampCosine(uvwz.x * 2.0f - 1.0f);
}


void getRMuMuSNuFromScatteringTextureCoord(
	AtmosphereParameters params, 
	float3 coord,
	out float r,
	out float mu,
	out float mu_s,
	out float nu,
	out bool intersect_ground 
)
{
	const float4 SCATTERING_TEXTURE_SIZE = float4(
						SCATTERING_TEXTURE_NU_SIZE - 1,
						SCATTERING_TEXTURE_MU_S_SIZE,
						SCATTERING_TEXTURE_MU_SIZE,
						SCATTERING_TEXTURE_R_SIZE);

	float coord_nu = floor(coord.x / float(SCATTERING_TEXTURE_MU_S_SIZE));
	float coord_mu_s = fmod(coord.x, float(SCATTERING_TEXTURE_MU_S_SIZE));
	float4 uvwz = float4(coord_nu, coord_mu_s, coord.y, coord.z) / SCATTERING_TEXTURE_SIZE;

	getRMuMuSNuFromScatteringTextureUvwz(params, uvwz, r,mu, mu_s, nu, intersect_ground);

	nu = clamp(nu, mu * mu_s - sqrt((1.0 - mu * mu) * (1.0 - mu_s * mu_s)),
		mu * mu_s + sqrt((1.0 - mu * mu) * (1.0 - mu_s * mu_s)));
}

void calculateSingleScatteringTexture(
	AtmosphereParameters params, 
	Texture2D transmittanceTexture,
	float3 coord, 
	out float3 rayleigh, 
	out float3 mie )
{
	float r;
	float mu;
	float mu_s;
	float nu;
	bool intersect_ground;
	getRMuMuSNuFromScatteringTextureCoord(params, coord, r, mu, mu_s, nu, intersect_ground);
	calculateSingleScattering(params, transmittanceTexture, r, mu, mu_s, mu, intersect_ground, rayleigh, mie);
}



[numthreads(1,1,1)]
void precomputeSingleScatteringTexture(uint3 globalIdx: SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx : SV_GroupID)
{
	float3 uvw = (float3(globalIdx) + 0.5f);
	float3 delta_rayleigh = 0;
	float3 delta_mie = 0;
	calculateSingleScatteringTexture(parameters, transmittance_texture, uvw, delta_rayleigh, delta_mie);

	rayleigh_scatteringTex_o[globalIdx] = float4(delta_rayleigh, 1.0f);
	mie_scatteringTex_o[globalIdx] = float4(delta_mie, 1.0f);

	scatteringTex_o[globalIdx] = float4(delta_rayleigh, 1.0f);
	single_mie_scatteringTex_o[globalIdx] = float4(delta_mie, 1.0f);
}

// direct irradiance =============================================================================================================

void getRMuSFromIrradianceTextureCoord(AtmosphereParameters params, float2 uv, out float r, out float mu_s)
{
	float x_mu_s = getUnitRangeFromTextureCoord(uv.x, IRRADIANCE_TEXTURE_WIDTH);
	float x_r = getUnitRangeFromTextureCoord(uv.y, IRRADIANCE_TEXTURE_HEIGHT);

	r = params.bottom_radius + x_r * (params.top_radius - params.bottom_radius);
	mu_s = clampCosine(2.0f * x_mu_s - 1.0f);
}


float3 calculateDirectIrradiance(AtmosphereParameters params, Texture2D transmittance_texture, float r, float mu_s)
{
	float alpha_s = params.sun_angular_radius ;
	float average_cosine_factor = mu_s < -alpha_s ? 0.0 : (mu_s > alpha_s ? mu_s :
		(mu_s + alpha_s) * (mu_s + alpha_s) / (4.0 * alpha_s));

	return params.solar_irradiance * 
		getTransmittanceToAtmosphereTop(params, transmittance_texture, r, mu_s) * average_cosine_factor;
}

float3 calculateDirectIrradianceTexture(AtmosphereParameters params, Texture2D transmittance_texture, float2 uv)
{
	float r; 
	float mu_s;
	getRMuSFromIrradianceTextureCoord(params, uv / IRRADIANCE_TEXTURE_SIZE, r, mu_s);

	return calculateDirectIrradiance(params, transmittance_texture, r, mu_s);
}



[numthreads(1, 1, 1)]
void precomputeDirectIrradiance(uint3 globalIdx: SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx : SV_GroupID)
{
	float2 uv = (float2(globalIdx.xy) + 0.5f) ;
	float3 result = calculateDirectIrradianceTexture( parameters, transmittance_texture, uv);
	deltaIrradianceTex_o[globalIdx.xy] = float4(result,1.0f);
	irradianceTex_o[globalIdx.xy] = 0;
}

// indirect irradiance =============================================================================================================

float4 getScatteringTextureUvwzFromRMuMuSNu(AtmosphereParameters params,
	float r, float mu, float mu_s, float nu,
	bool ray_r_mu_intersects_ground) {
	float H = sqrt(params.top_radius * params.top_radius -
		params.bottom_radius * params.bottom_radius);
	float rho =
		safeSqrt(r * r - params.bottom_radius * params.bottom_radius);
	float u_r = getTextureCoordFromUnitRange(rho / H, SCATTERING_TEXTURE_R_SIZE);

	float	r_mu = r * mu;
	float	discriminant =
		r_mu * r_mu - r * r + params.bottom_radius * params.bottom_radius;
	float u_mu;
	if (ray_r_mu_intersects_ground) {
		float d = -r_mu - safeSqrt(discriminant);
		float	d_min = r - params.bottom_radius;
		float	d_max = rho;
		u_mu = 0.5 - 0.5 * getTextureCoordFromUnitRange(
			d_max == d_min ? 0.0 : (d - d_min) / (d_max - d_min),
			SCATTERING_TEXTURE_MU_SIZE / 2);
	}
	else {
		float	d = -r_mu + safeSqrt(discriminant + H * H);
		float	d_min = params.top_radius - r;
		float	d_max = rho + H;
		u_mu = 0.5 + 0.5 * getTextureCoordFromUnitRange(
			(d - d_min) / (d_max - d_min), SCATTERING_TEXTURE_MU_SIZE / 2);
	}

	float d = distanceToAtmosphereTop(
		params, params.bottom_radius, mu_s);
	float	d_min = params.top_radius - params.bottom_radius;
	float	d_max = H;
	float	a = (d - d_min) / (d_max - d_min);
	float	A = -2.0 * params.mu_s_min * params.bottom_radius / (d_max - d_min);
	float	u_mu_s = getTextureCoordFromUnitRange(max(1.0 - a / A, 0.0) / (1.0 + a),
		SCATTERING_TEXTURE_MU_S_SIZE);
	float u_nu = (nu + 1.0) / 2.0; 
	return float4(u_nu, u_mu_s, u_mu, u_r);
}

float3 getScattering(AtmosphereParameters params,
	Texture3D scattering_texture,
	float r, float mu, float mu_s, float nu,
	bool ray_r_mu_intersects_ground) {
	float4 uvwz = getScatteringTextureUvwzFromRMuMuSNu(
		params, r, mu, mu_s, nu, ray_r_mu_intersects_ground);
	float	tex_coord_x = uvwz.x * float(SCATTERING_TEXTURE_NU_SIZE - 1);
	float	tex_x = floor(tex_coord_x);
	float	lerp = tex_coord_x - tex_x;  
	float3 uvw0 = float3((tex_x + uvwz.y) / float(SCATTERING_TEXTURE_NU_SIZE),
		uvwz.z, uvwz.w);
	float3 uvw1 = float3((tex_x + 1.0 + uvwz.y) / float(SCATTERING_TEXTURE_NU_SIZE),
		uvwz.z, uvwz.w);
	return float3(scattering_texture.SampleLevel(pointSampler, uvw0, 0).rgb ) * (1.0 - lerp) +
		float3(scattering_texture.SampleLevel(  pointSampler, uvw1, 0).rgb ) * lerp;
}

float rayleighPhaseFunction(float nu) {
	float k = 3.0 / (16.0 * PI );
	return(k * (1.0 + nu * nu));
}

float miePhaseFunction(float g, float nu) {
	float k = 3.0 / (8.0 * PI ) * (1.0 - g * g) / (2.0 + g * g);
	return(k * (1.0 + nu * nu) / pow(1.0 + g * g - 2.0 * g * nu, 1.5));
}


float3 getScattering(
	AtmosphereParameters params,
	Texture3D single_rayleigh_scattering_texture,
	Texture3D single_mie_scattering_texture,
	Texture3D multiple_scattering_texture,
	float r, float mu, float mu_s, float nu,
	bool ray_r_mu_intersects_ground,
	int scattering_order) {
	if (scattering_order == 1) {//单次散射
		float3 rayleigh = getScattering(
			params, single_rayleigh_scattering_texture, r, mu, mu_s, nu,
			ray_r_mu_intersects_ground);
		float3 mie = getScattering(
			params, single_mie_scattering_texture, r, mu, mu_s, nu,
			ray_r_mu_intersects_ground);
		return rayleigh * rayleighPhaseFunction(nu) +
			mie * miePhaseFunction(params.mie_phase_function_g, nu);
	}
	else {//多次散射
		return getScattering(params, multiple_scattering_texture, r, mu, mu_s, nu,
			ray_r_mu_intersects_ground);
	}
}

float3 calculateIndirectIrradiance(
	AtmosphereParameters params,
	float r, float mu_s, int scattering_order
)
{
	const int	SAMPLE_COUNT = 32;
	const float	dphi = PI / float(SAMPLE_COUNT);
	const float	dtheta = PI / float(SAMPLE_COUNT);

	float3 result = 0.0f;
	float3 omega_s = float3(sqrt(1.0 - mu_s * mu_s), 0.0, mu_s);
	for (int j = 0; j < SAMPLE_COUNT / 2; ++j) {
		float theta = (float(j) + 0.5) * dtheta;
		for (int i = 0; i < 2 * SAMPLE_COUNT; ++i) {
			float phi = (float(i) + 0.5) * dphi;
			float3 omega =
				float3(cos(phi) * sin(theta), sin(phi) * sin(theta), cos(theta));
			float domega = (dtheta ) * (dphi ) * sin(theta) ;
			float nu = dot(omega, omega_s);
			result += getScattering(params, delta_single_rayleigh_scattering_texture,
				delta_single_mie_scattering_texture, delta_multiple_scattering_texture,
				r, omega.z, mu_s, nu, false /* ray_r_theta_intersects_ground */,
				scattering_order) * omega.z * domega;
		}
	}
	return result;
}

float3 calculateIndirectIrradianceTexture(
	AtmosphereParameters params,
	float2 uv,
	int scattering_order)
{
	float r;
	float mu_s;
	getRMuSFromIrradianceTextureCoord(
		params,
		uv / IRRADIANCE_TEXTURE_SIZE,
		r,
		mu_s
	);
	return calculateIndirectIrradiance(
		params, 
		r, mu_s, scattering_order) ;
}

cbuffer MultipleScatteringConsts
{
	int scattering_order;
};


[numthreads(1, 1, 1)]
void precomputeIndirectIrradiance(uint3 globalIdx: SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx : SV_GroupID)
{
	float2 uv = float2(globalIdx.xy) + 0.5;

	float4 delta_irradiance = float4(calculateIndirectIrradianceTexture(
		parameters,uv, scattering_order), 1.0f);

	deltaIrradianceTex_o[globalIdx.xy] = delta_irradiance;
	irradianceTex_o[globalIdx.xy] += delta_irradiance;
}

// scattering density ==================================================================================================

bool rayIntersectsGround(AtmosphereParameters params,
	float r, float mu) {
	return(mu < 0.0 && r * r * (mu * mu - 1.0) +
		params.bottom_radius * params.bottom_radius >= 0.0);
}


float2 getIrradianceTextureCoordFromRMuS(float r, float mu_s)
{
	float x_r = (r - parameters.bottom_radius) / (parameters.top_radius - parameters.bottom_radius);
	float x_mu_s = mu_s * 0.5f + 0.5f;
	return float2(
		getTextureCoordFromUnitRange(x_mu_s, IRRADIANCE_TEXTURE_WIDTH),
		getTextureCoordFromUnitRange(x_r, IRRADIANCE_TEXTURE_HEIGHT));
}



float3 getIrradiance(float r, float mu_s)
{
	float2 uv = getIrradianceTextureCoordFromRMuS(r, mu_s);
	return irradiance_texture.SampleLevel(pointSampler, uv, 0).rgb;
}

float3 calculateScatteringDensity(
	AtmosphereParameters atmosphere,
	float r, float mu, float mu_s, float nu, int scattering_order) {
	float3 zenith_direction = float3(0.0, 0.0, 1.0);
	float3	omega = float3(sqrt(1.0 - mu * mu), 0.0, mu);
	float	sun_dir_x = omega.x == 0.0 ? 0.0 : (nu - mu * mu_s) / omega.x;
	float	sun_dir_y = sqrt(max(1.0 - sun_dir_x * sun_dir_x - mu_s * mu_s, 0.0));
	float3	omega_s = float3(sun_dir_x, sun_dir_y, mu_s);
	const int		SAMPLE_COUNT = 16;
	const float		dphi = PI / float(SAMPLE_COUNT);
	const float		dtheta = PI / float(SAMPLE_COUNT);
	float3 rayleigh_mie = 0.0;
	for (int l = 0; l < SAMPLE_COUNT; ++l) {
		float	theta = (float(l) + 0.5) * dtheta;
		float	cos_theta = cos(theta);
		float	sin_theta = sin(theta);
		bool ray_r_theta_intersects_ground = rayIntersectsGround(atmosphere, r, cos_theta);
		float	distance_to_ground = 0.0 ;
		float3 transmittance_to_ground = 0.0;
		float3 ground_albedo = 0.0;
		if (ray_r_theta_intersects_ground) {
			distance_to_ground = distanceToAtmosphereBottom(atmosphere, r, cos_theta);
			transmittance_to_ground =
				getTransmittance(atmosphere, transmittance_texture, r, cos_theta,
					distance_to_ground, true );
			ground_albedo = atmosphere.ground_albedo;
		}
		for (int m = 0; m < 2 * SAMPLE_COUNT; ++m) {
			float phi = (float(m) + 0.5) * dphi;
			float3 omega_i = float3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);
			float domega_i = (dtheta) * (dphi ) * sin(theta);
			float nu1 = dot(omega_s, omega_i);
			float3 incident_radiance = getScattering(atmosphere,
				delta_single_rayleigh_scattering_texture, delta_single_mie_scattering_texture,
				delta_multiple_scattering_texture, r, omega_i.z, mu_s, nu1,
				ray_r_theta_intersects_ground, scattering_order - 1);
			float3 ground_normal = normalize(zenith_direction * r + omega_i * distance_to_ground);
			float3 ground_irradiance = getIrradiance( 
				atmosphere.bottom_radius,
				dot(ground_normal, omega_s));
			incident_radiance += transmittance_to_ground *
				ground_albedo * (1.0 / (PI )) * ground_irradiance;

			float	nu2 = dot(omega, omega_i);
			float	rayleigh_density = getProfileDensity(
				atmosphere.rayleigh_density, r - atmosphere.bottom_radius);
			float mie_density = getProfileDensity(
				atmosphere.mie_density, r - atmosphere.bottom_radius);
			rayleigh_mie += incident_radiance * (
				atmosphere.rayleigh_scattering * rayleigh_density *
				rayleighPhaseFunction(nu2) +
				atmosphere.mie_scattering * mie_density *
				miePhaseFunction(atmosphere.mie_phase_function_g, nu2)) * domega_i;
		}
	}
	return rayleigh_mie;
}

float3 calculateScatteringDensityTexture(
	AtmosphereParameters atmosphere,
	float3 frag_coord, int scattering_order) {
	float	r;
	float	mu;
	float	mu_s;
	float	nu;
	bool	ray_r_mu_intersects_ground;
	getRMuMuSNuFromScatteringTextureCoord(atmosphere, frag_coord,
		r, mu, mu_s, nu, ray_r_mu_intersects_ground);
	return calculateScatteringDensity(atmosphere, r, mu, mu_s, nu,
		scattering_order);
}


[numthreads(1, 1, 1)]
void precomputeScatteringDensity(uint3 globalIdx: SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx : SV_GroupID)
{
	float3 uvw = float3(globalIdx) + 0.5;
	scattering_densityTex_o[globalIdx] = float4(calculateScatteringDensityTexture(parameters, uvw, scattering_order), 1.0f);
}

// multiple scattering =====================================================================================================
float3 calculateMultipleScattering(
	AtmosphereParameters atmosphere,
	float r, float mu, float mu_s, float nu,
	bool ray_r_mu_intersects_ground) {
	const int SAMPLE_COUNT = 50;
	float dx =	distanceToNearestAtmosphereBoundary(
			atmosphere, r, mu, ray_r_mu_intersects_ground) / float(SAMPLE_COUNT);
	float3 rayleigh_mie_sum = 0.0 ;
	for (int i = 0; i <= SAMPLE_COUNT; ++i) {
		float d_i = float(i) * dx;
		float r_i = clampRadius(atmosphere, sqrt(d_i * d_i + 2.0 * r * mu * d_i + r * r));
		float	mu_i = clampCosine((r * mu + d_i) / r_i);
		float	mu_s_i = clampCosine((r * mu_s + d_i * nu) / r_i);
		float3 rayleigh_mie_i =
			getScattering(
				atmosphere, delta_scattering_density_texture, r_i, mu_i, mu_s_i, nu,
				ray_r_mu_intersects_ground) *
			getTransmittance(
				atmosphere, transmittance_texture, r, mu, d_i,
				ray_r_mu_intersects_ground) * dx;
		float weight_i = (i == 0 || i == SAMPLE_COUNT) ? 0.5 : 1.0;
		rayleigh_mie_sum += rayleigh_mie_i * weight_i;
	}
	return rayleigh_mie_sum;
}


float3 calculateMultipleScatteringTexture(
	AtmosphereParameters atmosphere,
	float3 frag_coord, float nu) {
	float	r;
	float	mu;
	float	mu_s;
	bool	ray_r_mu_intersects_ground;
	getRMuMuSNuFromScatteringTextureCoord(atmosphere, frag_coord,
		r, mu, mu_s, nu, ray_r_mu_intersects_ground);
	return calculateMultipleScattering(atmosphere,  r, mu, mu_s, nu,
		ray_r_mu_intersects_ground);
}

[numthreads(1, 1, 1)]
void precomputeMultupleScattering(uint3 globalIdx: SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx : SV_GroupID)
{
	float3 uvw = float3(globalIdx)+0.5;
	float nu = 0;
	float3 delta_multiple_scattering = calculateMultipleScatteringTexture( parameters,  uvw, nu);
	multiple_scatteringTex_o[globalIdx] = float4(delta_multiple_scattering,1);
	scatteringTex_o[globalIdx] += float4(delta_multiple_scattering / rayleighPhaseFunction(nu), 1);
}


// final pass =============================================================================================================
cbuffer AtmosphericScatteringFinal
{
	float3 camera;
	float earth_radius;

	float3 sun_dir;
	float sun_cos;

	matrix invViewProj;

	float exposure;
	float sun_tan;
};

const static float3 earth_center = 0;




float3 getSunAndSkyIrradiance(float3 pt, float3 normal, out float3 sky_irradiance)
{
	float r = length(pt);
	float mu_s = dot(pt, sun_dir) / r;
	sky_irradiance = getIrradiance(r, mu_s) * (1.0f + dot(normal, pt) / r) * 0.5f;
	return 
		parameters.solar_irradiance * 
		getTransmittanceToSun(parameters, transmittance_texture, r, mu_s) *
		max(dot(normal, sun_dir), 0.0f);
}

float getSunVisibility(float3 pt, float3 sun_dir)
{
	return 1.0f;
}

float getSkyVisibility(float3 pt)
{
	return 1.0f;
}


float3 getExtrapolatedSingleMieScattering(
	AtmosphereParameters params, float4 scattering) {
	if (scattering.r == 0.0)return 0.0;
	return scattering.rgb * scattering.a / scattering.r *
		(params.rayleigh_scattering.r / params.mie_scattering.r) *
		(params.mie_scattering / params.rayleigh_scattering);
}

float3 getCombinedScattering(
	AtmosphereParameters params,
	Texture3D scattering_texture,
	Texture3D single_mie_scattering_texture,
	float r, float mu, float mu_s, float nu,
	bool ray_r_mu_intersects_ground,
	out float3 single_mie_scattering) {
	float4 uvwz = getScatteringTextureUvwzFromRMuMuSNu(
		params, r, mu, mu_s, nu, ray_r_mu_intersects_ground);
	float	tex_coord_x = uvwz.x * float(SCATTERING_TEXTURE_NU_SIZE - 1);
	float	tex_x = floor(tex_coord_x);
	float	lerp = tex_coord_x - tex_x;  
	float3	uvw0 = float3((tex_x + uvwz.y) / float(SCATTERING_TEXTURE_NU_SIZE),
		uvwz.z, uvwz.w);
	float3	uvw1 = float3((tex_x + 1.0 + uvwz.y) / float(SCATTERING_TEXTURE_NU_SIZE),
		uvwz.z, uvwz.w);
#ifdef COMBINED_SCATTERING_TEXTURES
	float4 combined_scattering =
		scattering_texture.SampleLevel( pointSampler, uvw0, 0).rgb * (1.0 - lerp) +
		scattering_texture.SampleLevel(pointSampler, uvw1, 0).rgb * lerp;
	float3 scattering = float3(combined_scattering);
	single_mie_scattering =
		getExtrapolatedSingleMieScattering(params, combined_scattering);
#else
	float3 scattering = float3(
		scattering_texture.SampleLevel(pointSampler, uvw0, 0).rgb * (1.0 - lerp) +
		scattering_texture.SampleLevel( pointSampler, uvw1, 0).rgb * lerp);
	single_mie_scattering = float3(
		single_mie_scattering_texture.SampleLevel( pointSampler, uvw0, 0).rgb * (1.0 - lerp) +
		single_mie_scattering_texture.SampleLevel( pointSampler, uvw1, 0).rgb * lerp);
#endif
	return scattering;
}

float3 getSkyRadiance(
	AtmosphereParameters params,
	float3 camera, float3 view_ray, float shadow_length,
	float3 sun_direction, out float3 transmittance) {
	float3 result_radiance = 0;
	float r = length(camera);
	float rmu = dot(camera, view_ray);
	float distance_to_top_atmosphere_boundary = -rmu -
		sqrt(rmu * rmu - r * r + params.top_radius * params.top_radius);
	if (distance_to_top_atmosphere_boundary > 0.0f ) {
		camera = camera + view_ray * distance_to_top_atmosphere_boundary;
		r = params.top_radius;
		rmu += distance_to_top_atmosphere_boundary;
	}

	if (r > params.top_radius) {
		transmittance = (1.0f);
		result_radiance = 0;
	}
	else
	{
		float	mu = rmu / r;
		float	mu_s = dot(camera, sun_direction) / r;
		float	nu = dot(view_ray, sun_direction);
		bool ray_r_mu_intersects_ground = rayIntersectsGround(params, r, mu);

		transmittance = ray_r_mu_intersects_ground ? (0.0) :
			getTransmittanceToAtmosphereTop(params, transmittance_texture, r, mu);
		float3	single_mie_scattering;
		float3	scattering;
		if (shadow_length == 0.0f) {
			scattering = getCombinedScattering(
				params, scattering_texture, single_mie_scattering_texture,
				r, mu, mu_s, nu, ray_r_mu_intersects_ground, single_mie_scattering);
		}
		else {
			float	d = shadow_length;
			float	r_p = clampRadius(params, sqrt(d * d + 2.0 * r * mu * d + r * r));
			float	mu_p = (r * mu + d) / r_p;
			float	mu_s_p = (r * mu_s + d * nu) / r_p;
			scattering = getCombinedScattering(
				params, scattering_texture, single_mie_scattering_texture,
				r_p, mu_p, mu_s_p, nu, ray_r_mu_intersects_ground,
				single_mie_scattering);
			float3 shadow_transmittance =
				getTransmittance(params, transmittance_texture,
					r, mu, shadow_length, ray_r_mu_intersects_ground);
			scattering = scattering * shadow_transmittance;
			single_mie_scattering = single_mie_scattering * shadow_transmittance;
		}
		result_radiance = scattering * rayleighPhaseFunction(nu)
			+ single_mie_scattering * miePhaseFunction(params.mie_phase_function_g, nu);
	}
	return result_radiance;
}

float3 getSkyRadianceToPoint( float3 camera, float3 pt, float shadow_length, float3 sun_direction, float3 transmittance) {
	float3	view_ray = normalize(pt - camera);
	float		r = length(camera);            
	float		rmu = dot(camera, view_ray);     
	float distance_to_top_atmosphere_boundary = -rmu -
		sqrt(rmu * rmu - r * r + parameters.top_radius * parameters.top_radius);
	if (distance_to_top_atmosphere_boundary > 0.0 ) {
		camera = camera + view_ray * distance_to_top_atmosphere_boundary;
		r = parameters.top_radius;
		rmu += distance_to_top_atmosphere_boundary;
	}
	float	mu = rmu / r;
	float	mu_s = dot(camera, sun_direction) / r;
	float	nu = dot(view_ray, sun_direction);
	float	d = length(pt - camera); 
	bool ray_r_mu_intersects_ground = rayIntersectsGround(parameters, r, mu);
	transmittance = getTransmittance(parameters, transmittance_texture,
		r, mu, d, ray_r_mu_intersects_ground);
	float3	single_mie_scattering;
	float3	scattering = getCombinedScattering(
		parameters, scattering_texture, single_mie_scattering_texture,
		r, mu, mu_s, nu, ray_r_mu_intersects_ground,
		single_mie_scattering);

	
	d = max(d - shadow_length, 0.0 );
	float r_p = clampRadius(parameters, sqrt(d * d + 2.0 * r * mu * d + r * r));
	float	mu_p = (r * mu + d) / r_p;
	float	mu_s_p = (r * mu_s + d * nu) / r_p;

	float3	single_mie_scattering_p;
	float3	scattering_p = getCombinedScattering(
		parameters, scattering_texture, single_mie_scattering_texture,
		r_p, mu_p, mu_s_p, nu, ray_r_mu_intersects_ground,
		single_mie_scattering_p);
	float3 shadow_transmittance = transmittance;
	if (shadow_length > 0.0 ) {
		shadow_transmittance = getTransmittance(parameters, transmittance_texture,
			r, mu, d, ray_r_mu_intersects_ground);
	}
	scattering = scattering - shadow_transmittance * scattering_p;
	single_mie_scattering =
		single_mie_scattering - shadow_transmittance * single_mie_scattering_p;
#ifdef COMBINED_SCATTERING_TEXTURES
	single_mie_scattering = getExtrapolatedSingleMieScattering(
		parameters, float4(scattering, single_mie_scattering.r));
#endif
	single_mie_scattering = single_mie_scattering *
		smoothstep(float(0.0), float(0.01), mu_s);
	return scattering * rayleighPhaseFunction(nu) + single_mie_scattering *
		miePhaseFunction(parameters.mie_phase_function_g, nu);
}

float3 getSolarRadiance() {
	return parameters.solar_irradiance /
		(PI * parameters.sun_angular_radius * parameters.sun_angular_radius);
}

half4 atmospheric_scattering(QuadInput input): SV_Target
{
	float4 pos = float4(input.uv * 2.0f - 1.0f , 1, 1);
	pos.y = -pos.y;
	pos = mul(  invViewProj, pos);
	pos /= pos.w;
	float3 view_dir = normalize(pos.xyz - camera.xyz);

	float3 p = camera - earth_center;
	float p_dot_v = dot(p, view_dir);
	float p_dot_p = dot(p,p);
	float ray_earth_center_squared_distance = p_dot_p - p_dot_v * p_dot_v;
	float distance_to_intersection = -p_dot_v - sqrt(
		earth_radius * earth_radius - ray_earth_center_squared_distance);

	float ground_alpha = 0;
	float3 ground_radiance = (0.0);

	if (distance_to_intersection > 0.0)
	{
		float3 pt = camera + view_dir * distance_to_intersection;
		float3 normal = normalize(pt - earth_center);
		float3 sky_irradiance;
		float3 sun_irradiance = getSunAndSkyIrradiance(pt - earth_center, normal, sky_irradiance);
		ground_radiance = 
			kGroundAlbedo * (1.0f / PI) * 
			(sun_irradiance * getSunVisibility(pt, sun_dir) + 
			 sky_irradiance * getSkyVisibility(pt));
		float shadow_float = 0.0f;

		float3 transmittance = 0;
		float3 in_scatter = getSkyRadianceToPoint(
			camera - earth_center, 
			pt - earth_center, 
			shadow_float, 
			sun_dir, 
			transmittance);
		ground_radiance = ground_radiance * transmittance + in_scatter;
		ground_alpha = 1.0;
	}

	float shadow_float = 0.0f;
	float3 transmittance;
	float3 radiance = getSkyRadiance(parameters, camera - earth_center, view_dir, shadow_float, sun_dir, transmittance);

	if (dot(view_dir, sun_dir) > sun_cos)
		radiance += transmittance * getSolarRadiance();

	radiance = lerp(radiance, ground_radiance, ground_alpha);
	float4 color = 1;
	color.rgb = 1.0f - exp(-radiance * exposure);
	return color;
}