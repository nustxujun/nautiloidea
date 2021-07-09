cbuffer Constants
{
	matrix view;
	matrix proj;
	float3 campos;
	float time;
};

struct VSOutput
{
	float3 position: POSITION;
	float2 uv: TEXCOORD;
};

VSOutput vs(uint id:SV_VertexID)
{
	VSOutput o;
	float x = float(id % 2);
	float y = float(id / 2 % 2);
	o.position = float3(x * 2.0f - 1.0f, 0, 1.0f - y * 2.0f);
	o.uv = float2(x, y);
	return o;
}

struct PatchTess
{
	float EdgeTess[4]   : SV_TessFactor;
	float InsideTess[2] : SV_InsideTessFactor;
};

PatchTess ConstantHS(InputPatch<VSOutput, 4> patch, uint patchID : SV_PrimitiveID)
{
	float3 center = 0.25f * (patch[0].position + patch[1].position + patch[2].position + patch[3].position);

	float d = distance(center, campos);

	
	PatchTess pt;

	const float d0 = 0.0f;
	const float d1 = 50.0f;
	float tess = 64.0f * saturate((d1 - d) / (d1 - d0));

	// Uniformly tessellate the patch.

	pt.EdgeTess[0] = tess;
	pt.EdgeTess[1] = tess;
	pt.EdgeTess[2] = tess;
	pt.EdgeTess[3] = tess;

	pt.InsideTess[0] = tess;
	pt.InsideTess[1] = tess;

	return pt;
}


struct HSOutput
{
	float3 PosL : POSITION;
	float3 norm: NORMAL;
};

[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(4)]
[patchconstantfunc("ConstantHS")]
[maxtessfactor(64.0f)]
HSOutput hs(InputPatch<VSOutput, 4> p,
	uint i : SV_OutputControlPointID,
	uint patchId : SV_PrimitiveID)
{
	HSOutput hout;

	hout.PosL = p[i].position;
	return hout;
}

struct DSOutput
{
	float4 pos : SV_POSITION;
	float3 norm: NORMAL;
	float2 uv: TEXCOORD;
};


float3 calPos(float3 p1, float3 p2, float3 p3, float3 p4, float2 uv)
{
	float3 v1 = lerp(p1, p2, uv.x);
	float3 v2 = lerp(p3, p4, uv.x);
	float3 p = lerp(v1, v2, uv.y);

	return p;
}

float3 calrealPos(float3 pos)
{
	pos.y = sin(pos.x * 10 + time) * 0.1 ;//+ sin(pos.z * 5 + time) * 0.1;
	return pos;
}
// The domain shader is called for every vertex created by the tessellator.  
// It is like the vertex shader after tessellation.
[domain("quad")]
DSOutput ds(PatchTess patchTess,
	float2 uv : SV_DomainLocation,
	const OutputPatch<HSOutput, 4> quad)
{
	DSOutput dout;

	// Bilinear interpolation.
	float3 p = calPos(quad[0].PosL, quad[1].PosL, quad[2].PosL, quad[3].PosL, uv);

	float4 pos  = float4(calrealPos(p), 1);
	

	float3 nx = calPos(quad[0].PosL, quad[1].PosL, quad[2].PosL, quad[3].PosL, uv + float2(1 / 64.0,0));
	float3 ny = calPos(quad[0].PosL, quad[1].PosL, quad[2].PosL, quad[3].PosL, uv + float2(0, 1 / 64.0));
	nx = calrealPos(nx);
	ny= calrealPos(ny);

	dout.norm = normalize(cross(nx - pos, ny - pos));

	pos = mul(pos, view);
	pos = mul(pos, proj);

	dout.pos = pos;
	dout.uv = uv;


	dout.norm = dout.norm * 0.5 +  0.5;
	return dout;
}

Texture2D albedo;
Texture2D flowmap;
sampler linear_wrap;
float4 ps(DSOutput input) : SV_TARGET
{
	float2 uv = input.uv;
	float4 vel = flowmap.Sample(linear_wrap, uv);
	vel = vel * 2.0f - 1.0f;
	float time1= (time * 0.25f);
	float time2 = (time * 0.25f) + 0.5f;
	float2 uv1 = uv + vel.rg * time1 * float2(1,1);
	float2 uv2 = uv + vel.rg * time2 * float2(1,1);
	float4 color1 = albedo.Sample(linear_wrap, uv1);
	float4 color2 = albedo.Sample(linear_wrap, uv2);

	float4 color = lerp(color1, color2, frac(time1));

	return color1;
}

//float4 ps(VSOutput input) :SV_TARGET
//{
//	return float4(1,1,1,1);
//}