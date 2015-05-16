float4x4 matCombined;
float4x4 matWorld;
texture texDiffuse;
sampler2D samplerDiffuse = sampler_state
{
	texture = <texDiffuse>;
	MipFilter = Linear;
	MinFilter = Linear;
	MagFilter = Linear;
};
// mg-a
float4 fogColor;
float4 fogParam; //(x=fogStart,y=fogEnd,z=(fogEnd-fogStart),a=?)
// dodatkowy kolor
float4 tint;
// o¶wietlenie ambient
float3 ambientColor;
// o¶wietlenie na zewn-trz
float3 lightDir;
float3 lightColor;
// o¶wietlenie wewn-trz
struct Light
{
	float4 pos;
	float3 color;	
};
Light lights[3];

//------------------------------------------------------------------------------
// WEJ¦CIE DO VERTEX SHADERA
struct MESH_INPUT
{
  float3 pos : POSITION;
  float3 normal : NORMAL;
  float2 tex : TEXCOORD0;
};

//*************************************************************************************************************************************
// O¦WIETLENIE PUNKTOWE, U¯YWANE W PODZIEMIACH, DO 3 ¦WIATE£ NA OBIEKT
//*************************************************************************************************************************************

//------------------------------------------------------------------------------
// WYJ¦CIE Z VERTEX SHADERA
struct MESH_OUTPUT
{
	float4 pos : POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : TEXCOORD1;
	float posViewZ : TEXCOORD2;
	float3 posWorld : TEXCOORD3;
};

//------------------------------------------------------------------------------
// VERTEX SHADER
void vs_mesh(in MESH_INPUT In, out MESH_OUTPUT Out)
{
	Out.tex = In.tex;
	Out.pos = mul(float4(In.pos,1), matCombined);
	Out.normal = mul(In.normal, matWorld);
	Out.posWorld = mul(float4(In.pos,1), matWorld);
	Out.posViewZ = Out.pos.w;
}

//------------------------------------------------------------------------------
// PIXEL SHADER
float4 ps_mesh(in MESH_OUTPUT In) : COLOR0
{
	float4 tex = tex2D(samplerDiffuse, In.tex);
	float3 diffuse = float3(0,0,0);
	float lightage = 0;
	for(int i=0; i<3; ++i)
	{
		float3 lightVec = normalize(lights[i].pos - In.posWorld);
		float dist = distance(lights[i].pos, In.posWorld);
		float light = clamp(dot(lightVec, In.normal),0,1) * clamp((1-(dist/lights[i].pos.w)),0,1);
		diffuse += light*lights[i].color;
		lightage += light;
	}
	float fog = min(saturate((In.posViewZ-fogParam.x)/fogParam.z), 1-min(lightage, 1));
	return float4(lerp(tex.xyz * saturate(diffuse + ambientColor) * tint, fogColor, fog), tex.w);
}

//------------------------------------------------------------------------------
// TECHNIKA
technique mesh
{
	pass pass0
	{
		VertexShader = compile VS_VERSION vs_mesh();
		PixelShader = compile PS_VERSION ps_mesh();
	}
}

//*************************************************************************************************************************************
// O¦WIETLENIE KIERUNKOWE, U¯YWANE NA ZEWN¡TRZ
//*************************************************************************************************************************************

//------------------------------------------------------------------------------
// WYJ¦CIE Z VERTEX SHADERA
struct MESH_OUTPUT_DIR
{
	float4 pos : POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : TEXCOORD1;
	float posViewZ : TEXCOORD2;
};

//------------------------------------------------------------------------------
// VERTEX SHADER
void vs_mesh_dir(in MESH_INPUT In, out MESH_OUTPUT_DIR Out)
{
	Out.tex = In.tex;
	Out.pos = mul(float4(In.pos,1), matCombined);
	Out.normal = mul(In.normal, matWorld);
	Out.posViewZ = Out.pos.w;
}

//------------------------------------------------------------------------------
// PIXEL SHADER
float4 ps_mesh_dir(in MESH_OUTPUT_DIR In) : COLOR0
{
	float4 tex = tex2D(samplerDiffuse, In.tex);
	float3 diffuse = lightColor * saturate(dot(lightDir, In.normal));
	float fog = saturate((In.posViewZ-fogParam.x)/fogParam.z);
	return float4(lerp(tex.xyz * saturate(lightColor*diffuse + ambientColor) * tint, fogColor, fog), tex.w);
}

//------------------------------------------------------------------------------
// TECHNIKA
technique mesh_dir
{
	pass pass0
	{
		VertexShader = compile VS_VERSION vs_mesh_dir();
		PixelShader = compile PS_VERSION ps_mesh_dir();
	}
}

//*************************************************************************************************************************************
// BEZ O¦WIETLENIA
//*************************************************************************************************************************************

//------------------------------------------------------------------------------
// WYJ¦CIE Z VERTEX SHADERA
struct MESH_OUTPUT_SIMPLE
{
	float4 pos : POSITION;
	float2 tex : TEXCOORD0;
};

//------------------------------------------------------------------------------
// VERTEX SHADER
void vs_mesh_simple(in MESH_INPUT In, out MESH_OUTPUT_SIMPLE Out)
{
	Out.tex = In.tex;
	Out.pos = mul(float4(In.pos,1), matCombined);
}

//------------------------------------------------------------------------------
// PIXEL SHADER
float4 ps_mesh_simple(in MESH_OUTPUT_SIMPLE In) : COLOR0
{
	return tex2D(samplerDiffuse, In.tex);
}

//------------------------------------------------------------------------------
// TECHNIKA
technique mesh_simple
{
	pass pass0
	{
		VertexShader = compile VS_VERSION vs_mesh_simple();
		PixelShader = compile PS_VERSION ps_mesh_simple();
	}
}

//*************************************************************************************************************************************
// TYLKO KOLOR
//*************************************************************************************************************************************

//------------------------------------------------------------------------------
// VERTEX SHADER
float4 vs_mesh_simple2(in MESH_INPUT In) : POSITION
{
	return mul(float4(In.pos,1), matCombined);
}

//------------------------------------------------------------------------------
// PIXEL SHADER
float4 ps_mesh_simple2() : COLOR0
{
	return tint;
}

//------------------------------------------------------------------------------
// TECHNIKA
technique mesh_simple2
{
	pass pass0
	{
		VertexShader = compile VS_VERSION vs_mesh_simple2();
		PixelShader = compile PS_VERSION ps_mesh_simple2();
	}
}

//*************************************************************************************************************************************
// EKSPLOZJA
//*************************************************************************************************************************************

//------------------------------------------------------------------------------
// PIXEL SHADER
float4 ps_explo(in MESH_OUTPUT_SIMPLE In) : COLOR0
{
	float4 tex = tex2D(samplerDiffuse, In.tex);
	return float4(tex.xyz, tint.w);
}

//------------------------------------------------------------------------------
// TECHNIKA
technique mesh_explo
{
	pass pass0
	{
		VertexShader = compile VS_VERSION vs_mesh_simple();
		PixelShader = compile PS_VERSION ps_explo();
	}
}
