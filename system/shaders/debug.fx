float4x4 matCombined;

struct ColorVertex
{
  float3 pos : POSITION;
  float4 color : COLOR;
};

struct ColorVertexOutput
{
	float4 pos : POSITION;
	float4 color : COLOR;
};

void vs_simple(in ColorVertex In, out ColorVertexOutput Out)
{
	Out.pos = mul(float4(In.pos,1), matCombined);
	Out.color = In.color;
}

float4 ps_simple(in ColorVertexOutput In) : COLOR0
{
	return In.color;
}

technique simple
{
	pass pass0
	{
		VertexShader = compile VS_VERSION vs_simple();
		PixelShader = compile PS_VERSION ps_simple();
	}
}
