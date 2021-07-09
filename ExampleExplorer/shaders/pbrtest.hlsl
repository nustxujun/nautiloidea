#include "Common.hlsl"
#include "pbr.hlsl"

cbuffer PSConstant
{

}

sampler linear_wrap;
sampler point_wrap;
sampler anisotropic_wrap;

Texture2D albedo;
Texture2D roughness;
Texture2D metallic;
Texture2D normal;
static const float pi = 3.14159265358;
float4 ps(PSInput input) : SV_TARGET
{
	float2 uv = input.uv;

	float3x3 TBN = float3x3(input.tangent, input.binormal, input.normal);
	float3 n = normalize(normal.Sample(linear_wrap, uv).rgb * 2.0f - 1.0f);
	n= mul(n, TBN);

	float3 a = albedo.Sample(linear_wrap,uv).rgb;
	
	float r = roughness.Sample(linear_wrap,uv).r;
	float  m = metallic.Sample(linear_wrap, uv).r;

	float3 L = -sundir;
	float3 V = -camdir.xyz;
	float3 brdf = directBRDF(r, m, F0_DEFAULT, a, n, L, V);
	return  float4(brdf,1) * suncolor;
}
