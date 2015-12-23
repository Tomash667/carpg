// SHADER DO TERENU
//------------------------------------------------------------------------------
// zmienne
float4x4 matCombined;
float4x4 matWorld;
texture texBlend;
texture tex0;
texture tex1;
texture tex2;
texture tex3;
texture tex4;

sampler samplerBlend = sampler_state
{
	Texture = <texBlend>;
	AddressU = Clamp;
	AddressV = Clamp;
	MipFilter = None;
	MinFilter = Linear;
	MagFilter = Linear;
};

sampler sampler0 = sampler_state
{
	Texture = <tex0>;
	MipFilter = Linear;
	MinFilter = Linear;
	MagFilter = Linear;
};

sampler sampler1 = sampler_state
{
	Texture = <tex1>;
	MipFilter = Linear;
	MinFilter = Linear;
	MagFilter = Linear;
};

sampler sampler2 = sampler_state
{
	Texture = <tex2>;
	MipFilter = Linear;
	MinFilter = Linear;
	MagFilter = Linear;
};

sampler sampler3 = sampler_state
{
	Texture = <tex3>;
	MipFilter = Linear;
	MinFilter = Linear;
	MagFilter = Linear;
};

sampler sampler4 = sampler_state
{
	Texture = <tex4>;
	MipFilter = Linear;
	MinFilter = Linear;
	MagFilter = Linear;
};

float4 colorAmbient;
float4 colorDiffuse;
float3 lightDir;
float4 fogColor;
float4 fogParam; //(x=fogStart,y=fogEnd,z=(fogEnd-fogStart),a=?)

//------------------------------------------------------------------------------
// WEJŒCIE DO VERTEX SHADERA
struct TERRAIN_INPUT
{
	float3 pos : POSITION;
	float3 normal : NORMAL;
	float2 tex : TEXCOORD0;
	float2 texBlend : TEXCOORD1;
};

//------------------------------------------------------------------------------
// WYJŒCIE Z VERTEX SHADERA
struct TERRAIN_OUTPUT
{
	float4 pos : POSITION;
	float2 tex : TEXCOORD0;
	float2 texBlend : TEXCOORD1;
	float3 normal : TEXCOORD2;
	float posViewZ : TEXCOORD3;
};

//------------------------------------------------------------------------------
// VERTEX SHADER
void vs_terrain(in TERRAIN_INPUT In, out TERRAIN_OUTPUT Out)
{
	Out.pos = mul(float4(In.pos,1), matCombined);
	Out.tex = In.tex;
	Out.texBlend = In.texBlend;
	Out.normal = mul(In.normal, matWorld);
	Out.posViewZ = Out.pos.w;
}

//------------------------------------------------------------------------------
// PIXEL SHADER
float4 ps_terrain(in TERRAIN_OUTPUT In) : COLOR0
{
	float4 a = tex2D(samplerBlend, In.texBlend);
	float4 c0 = tex2D(sampler0, In.tex);
	float4 c1 = tex2D(sampler1, In.tex);
	float4 c2 = tex2D(sampler2, In.tex);
	float4 c3 = tex2D(sampler3, In.tex);
	float4 c4 = tex2D(sampler4, In.tex);
	
	float4 texColor = lerp(c0,c1,a.b);
	texColor = lerp(texColor,c2,a.g);
	texColor = lerp(texColor,c3,a.r);
	texColor = lerp(texColor,c4,a.a);
	
	float3 diffuse = colorDiffuse * saturate(dot(lightDir,In.normal));

	float4 new_texel = float4( texColor.xyz * saturate(colorAmbient +
		diffuse), texColor.w );

	float l = saturate((In.posViewZ-fogParam.x)/fogParam.z);
	new_texel = lerp(new_texel, fogColor, l);

	return new_texel;
}

//------------------------------------------------------------------------------
// TECHNIKA
technique terrain
{
	pass pass0
	{
		VertexShader = compile VS_VERSION vs_terrain();
		PixelShader = compile PS_VERSION ps_terrain();
	}
}
