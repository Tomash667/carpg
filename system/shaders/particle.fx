// particle shader
//------------------------------------------------------------------------------
float4x4 matCombined : ViewProjection;
texture tex0;

sampler2D sampler0 = sampler_state
{
	texture = <tex0>;
	MipFilter = Linear;
	MinFilter = Linear;
	MagFilter = Linear;
};

//------------------------------------------------------------------------------
struct VS_INPUT
{
	float3 pos : POSITION;
	float2 tex : TEXCOORD;
	float4 color : COLOR;
};
struct VS_OUTPUT
{
	float4 pos : POSITION;
	float2 tex : TEXCOORD;
	float4 color : COLOR;
};

//------------------------------------------------------------------------------
void vs_particle(in VS_INPUT In, out VS_OUTPUT Out)
{
	Out.pos = mul(float4(In.pos,1), matCombined);
	Out.tex = In.tex;
	Out.color = In.color;
}

float4 ps_particle(in VS_OUTPUT In) : COLOR0
{
	return tex2D(sampler0, In.tex) * In.color;
}

//------------------------------------------------------------------------------
technique particle
{
	pass
	{
		VertexShader = compile VS_VERSION vs_particle();
		PixelShader = compile PS_VERSION ps_particle();
	}
}

struct VS_INPUT2
{
	float3 pos : POSITION;
	float4 color : COLOR;
};

struct VS_OUTPUT2
{
	float4 pos : POSITION;
	float4 color : COLOR;
};

void vs_trail(in VS_INPUT2 In, out VS_OUTPUT2 Out)
{
	Out.pos = mul(float4(In.pos, 1), matCombined);
	Out.color = In.color;
}

float4 ps_trail(in VS_OUTPUT2 In) : COLOR0
{
	return In.color;
}

technique trail
{
	pass
	{
		VertexShader = compile VS_VERSION vs_trail();
		PixelShader = compile PS_VERSION ps_trail();
	}
}
