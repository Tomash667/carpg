float2 size;
texture tex0;

sampler sampler0 = sampler_state
{
	Texture = <tex0>;
	MipFilter = None;
	MinFilter = Linear;
	MagFilter = Linear;
	AddressU = Clamp;
	AddressV = Clamp;
};

struct VERTEX_INPUT
{
	float3 pos : POSITION;
	float2 tex : TEXCOORD0;
	float4 color : COLOR;
};

struct VERTEX_OUTPUT
{
	float4 pos : POSITION;
	float2 tex : TEXCOORD0;
	float4 color : COLOR;
};

void vs(in VERTEX_INPUT In, out VERTEX_OUTPUT Out)
{
	// fix half pixel problem
	Out.pos.x = ((In.pos.x - 0.5f) / (size.x * 0.5f)) - 1.0f;
	Out.pos.y = -(((In.pos.y - 0.5f) / (size.y * 0.5f)) - 1.0f);
	Out.pos.z = 0.f;
	Out.pos.w = 1.f;
	Out.tex = In.tex;
	Out.color = In.color;
}

float4 ps(in VERTEX_OUTPUT In) : COLOR0
{
	float4 c = tex2D(sampler0, In.tex);
	return c * In.color;
}

float4 ps2(in VERTEX_OUTPUT In) : COLOR0
{
	return In.color;
}

technique gui
{
	pass pass0
	{
		VertexShader = compile VS_VERSION vs();
		PixelShader = compile PS_VERSION ps();
	}
}

technique gui2
{
	pass pass0
	{
		VertexShader = compile VS_VERSION vs();
		PixelShader = compile PS_VERSION ps2();
	}
}
