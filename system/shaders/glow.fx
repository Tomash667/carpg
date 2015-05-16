float4x4 matCombined;
float4x4 matBones[32];
float4 color;

//******************************************************************************
struct MESH_INPUT
{
  float3 pos : POSITION;
  float2 tex : TEXCOORD0;
  float3 normal : NORMAL;
};

//******************************************************************************
float4 vs_mesh(in MESH_INPUT In) : POSITION
{
	return mul(float4(In.pos,1), matCombined);
}

//******************************************************************************
struct ANI_INPUT
{
  float3 pos : POSITION;
  float2 tex : TEXCOORD0;
  float3 normal : NORMAL;
	half weight : BLENDWEIGHT0;
	int4 indices : BLENDINDICES0;
};

//******************************************************************************
float4 vs_ani(in ANI_INPUT In) : POSITION
{
	float3 pos = mul(float4(In.pos,1), matBones[In.indices[0]]) * In.weight;
	pos += mul(float4(In.pos,1), matBones[In.indices[1]]) * (1-In.weight);
	return mul(float4(pos,1), matCombined);
}

//******************************************************************************
float4 ps() : COLOR0
{
	return color;
}

//******************************************************************************
technique mesh
{
	pass pass0
	{
		VertexShader = compile VS_VERSION vs_mesh();
		PixelShader = compile PS_VERSION ps();
	}
}

//******************************************************************************
technique ani
{
	pass pass0
	{
		VertexShader = compile VS_VERSION vs_ani();
		PixelShader = compile PS_VERSION ps();
	}
}
