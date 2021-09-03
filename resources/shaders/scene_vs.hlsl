#include "common.hlsl"

#define ENTRY_POINT vs
#define TARGET vs_5_0

cbuffer PrivateConstants
{
	matrix world;

	matrix nworld;
};

struct VSInput
{
	float3 position : POSITION;
	float2 uv : TEXCOORD;
	float3 normal: NORMAL0;
	float3 tangent: NORMAL1;
	float3 binormal: NORMAL2;
};

PSInput vs(VSInput input)
{
	PSInput result;

	float4 worldpos = mul(float4(input.position,1), world);
	result.worldPos = worldpos;

	float4 viewpos = mul(worldpos,view );
	result.position = mul(viewpos, proj);
	result.uv = input.uv;


	result.normal = mul(float4(input.normal, 0), nworld);
	result.normal.xyz = normalize(result.normal.xyz);
	result.tangent = mul(float4(input.tangent, 0), nworld);
	result.tangent.xyz = normalize(result.tangent.xyz);
	result.binormal = mul(float4(input.binormal, 0), nworld);
	result.binormal.xyz = normalize(result.binormal.xyz);
	return result;
}

