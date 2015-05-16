// Skybox shader
texture tex0;
float4x4 matCombined;

sampler sampler0 = sampler_state
{
	Texture = <tex0>;
	MipFilter = Linear;
	MinFilter = Linear;
	MagFilter = Linear;
	AddressU = Clamp;
	AddressV = Clamp;
};

struct VS_INPUT
{
	float3 pos : POSITION;
	float2 tex : TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 pos : POSITION;
	float2 tex : TEXCOORD0;
};

void vs_skybox(VS_INPUT In, out VS_OUTPUT Out)
{
	Out.pos = mul(float4(In.pos, 1), matCombined);
	Out.tex = In.tex;
}

float4 ps_skybox(VS_OUTPUT In) : COLOR0
{
	return tex2D(sampler0, In.tex);
}

technique skybox
{
	pass pass0
	{
		VertexShader = compile VS_VERSION vs_skybox();
		PixelShader = compile PS_VERSION ps_skybox();
	}
}
