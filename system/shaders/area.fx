float4x4 matCombined;
float4 color;
float4 playerPos;
float range;

struct VERTEX_OUT
{
	float4 pos : POSITION;
	float3 realPos : TEXCOORD0;
};

//------------------------------------------------------------------------------
// VERTEX SHADER
void vs_area(in float3 pos : POSITION, out VERTEX_OUT Out)
{
	Out.pos = mul(float4(pos,1), matCombined);
	Out.realPos = pos;
}

//------------------------------------------------------------------------------
// PIXEL SHADER
float4 ps_area(in VERTEX_OUT In) : COLOR0
{
	float dist = distance(In.realPos, playerPos);
	clip(range - dist);
	return float4(color.rgb, lerp(color.a, 0.f, dist/range));
}

//------------------------------------------------------------------------------
// TECHNIKA
technique area
{
	pass pass0
	{
		VertexShader = compile VS_VERSION vs_area();
		PixelShader = compile PS_VERSION ps_area();
	}
}
