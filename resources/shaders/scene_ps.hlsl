#include "Common.hlsl"
#include "pbr.hlsl"


#define ENTRY_POINT ps
#define TARGET ps_5_0

cbuffer PrivateConstants
{

}

Texture2D albedo;
sampler linear_wrap;

float4 ps(PSInput input): SV_TARGET
{
	return albedo.Sample(linear_wrap, input.uv);
	// return float4(0,1,0,1);
	//float4 base = float4(0.120000, 0.102846, 0.091800, 1.000000);
	//float4 seat = float4(0.974138,0.337885,0.034461,1.000000);
	//float4 metal = float4(0.913793, 0.864979, 0.718538, 1.000000);
	////float4 color = lerp(lerp(base, seat, params.b), metal,params.g) * params.r;
	//float4 color = lerp(seat, metal, params.g) * params.r;
	//return  color;
}
