float4x4 matCombined;
float4x4 matBones[32];
float4 color;
texture texDiffuse;
sampler2D samplerDiffuse = sampler_state
{
	texture = <texDiffuse>;
	MipFilter = Linear;
	MinFilter = Linear;
	MagFilter = Linear;
};

//******************************************************************************
struct MESH_INPUT
{
	float3 pos : POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
};

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
struct MESH_OUTPUT
{
	float4 pos : POSITION;
	float2 tex : TEXCOORD0;
};

//******************************************************************************
void vs_mesh(in MESH_INPUT In, out MESH_OUTPUT Out)
{
	Out.pos = mul(float4(In.pos,1), matCombined);
	Out.tex = In.tex;
}

//******************************************************************************
void vs_ani(in ANI_INPUT In, out MESH_OUTPUT Out)
{
	float3 pos = mul(float4(In.pos,1), matBones[In.indices[0]]) * In.weight;
	pos += mul(float4(In.pos,1), matBones[In.indices[1]]) * (1-In.weight);
	Out.pos = mul(float4(pos,1), matCombined);
	Out.tex = In.tex;
}

//******************************************************************************
float4 ps(in MESH_OUTPUT In) : COLOR0
{
	float4 tex = tex2D(samplerDiffuse, In.tex);
	return float4(color.rgb, tex.w * color.w);
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
