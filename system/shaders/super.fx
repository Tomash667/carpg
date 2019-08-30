/* SUPER SHADER
--------------------------------------------------------------------------------
Switches:
- ANIMATED
- HAVE_BINORMALS
- FOG
- SPECULAR_MAP
- NORMAL_MAP
- POINT_LIGHT
- DIR_LIGHT
- ALPHA_BLENDING

Defines:
- VS_VERSION
- PS_VERSION
- LIGHTS
*/

#if defined(POINT_LIGHT) && defined(DIR_LIGHT)
#	error "Mixed lighting not supported yet!"
#endif

#if defined(NORMAL_MAP) && !defined(HAVE_BINORMALS)
#	error "Normal mapping require binormals!"
#endif

//******************************************************************************
// macierze
shared float4x4 mat_combined;
shared float4x4 mat_world;
shared float4x4 mat_bones[32];

// dodatkowy kolor/alpha
shared float4 tint;

// oswietlenie ambient
shared float3 ambient_color;

// mgła
shared float4 fog_color;
shared float4 fog_params; //(x=fogStart,y=fogEnd,z=(fogEnd-fogStart),a=?)

// oswietlenie na zewnatrz
shared float3 light_dir;
shared float3 light_color;

// oswietlenie wewnatrz
struct Light
{
	float4 pos;
	float3 color;	
};
shared Light lights[3];

shared float3 camera_pos;
shared float3 specular_color;
shared float specular_intensity;
shared float specular_hardness;

// tekstury
shared texture tex_diffuse;
shared texture tex_normal;
shared texture tex_specular;

// samplery
sampler2D sampler_diffuse = sampler_state
{
	texture = <tex_diffuse>;
	MipFilter = Linear;
	MinFilter = Linear;
	MagFilter = Linear;
};
sampler2D sampler_normal = sampler_state
{
	texture = <tex_normal>;
	MipFilter = Linear;
	MinFilter = Linear;
	MagFilter = Linear;
};
sampler2D sampler_specular = sampler_state
{
	texture = <tex_specular>;
	MipFilter = Linear;
	MinFilter = Linear;
	MagFilter = Linear;
};

//******************************************************************************
struct MESH_INPUT
{
	float3 pos : POSITION;
#ifdef ANIMATED
	half weight : BLENDWEIGHT0;
	int4 indices : BLENDINDICES0;
#endif
	float3 normal : NORMAL;
	float2 tex : TEXCOORD0;
#ifdef HAVE_BINORMALS
	float3 tangent : TANGENT;
	float3 binormal : BINORMAL;
#endif
};

//******************************************************************************
struct MESH_OUTPUT
{
	float4 pos : POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : TEXCOORD1;
#ifdef FOG
	float pos_view_z : TEXCOORD2;
#endif
#ifdef POINT_LIGHT
	float3 pos_world : TEXCOORD3;
#endif
#ifdef NORMAL_MAP
	float3 tangent : TEXCOORD4;
	float3 binormal : TEXCOORD5;
#endif
	float3 view_dir : TEXCOORD6;
};

//******************************************************************************
void vs_mesh(in MESH_INPUT In, out MESH_OUTPUT Out)
{
	// uv tekstury
	Out.tex = In.tex;
	
	// pozycja
#ifdef ANIMATED
	float3 pos = mul(float4(In.pos,1), mat_bones[In.indices[0]]) * In.weight;
	pos += mul(float4(In.pos,1), mat_bones[In.indices[1]]) * (1-In.weight);
	Out.pos = mul(float4(pos,1), mat_combined);
#else
	float3 pos = In.pos;
	Out.pos = mul(float4(In.pos,1), mat_combined);
#endif

	// normalna / kolor
#if defined(ANIMATED)
	float3 normal = mul(float4(In.normal,1), mat_bones[In.indices[0]]) * In.weight;
	normal += mul(float4(In.normal,1), mat_bones[In.indices[1]]) * (1-In.weight);
	Out.normal = mul(normal, mat_world);
#else
	Out.normal = mul(In.normal, mat_world);
#endif

	// odleglosc dla mgly
#ifdef FOG
	Out.pos_view_z = Out.pos.w;
#endif
	
	// pozycja w odniesieniu do pozycji swiata
#ifdef POINT_LIGHT
	Out.pos_world = mul(float4(pos,1), mat_world);
#endif

#ifdef NORMAL_MAP
	Out.tangent = normalize(mul(In.tangent,mat_world));
	Out.binormal = normalize(mul(In.binormal,mat_world));
#endif

#ifdef POINT_LIGHT
	Out.view_dir = camera_pos - Out.pos_world;
#else
	Out.view_dir = camera_pos - mul(float4(pos,1), mat_world);
#endif
}

//******************************************************************************
float4 ps_mesh(in MESH_OUTPUT In) : COLOR0
{
	float4 tex = tex2D(sampler_diffuse, In.tex) * tint;
	
#if defined(FOG)
	float fog = saturate((In.pos_view_z-fog_params.x)/fog_params.z);
#endif

#if defined(NORMAL_MAP)
	float3 bump = tex2D(sampler_normal, In.tex) * 2.f - 1.f;
	float3 normal = normalize(bump.x*In.tangent + (-bump.y)*In.binormal + bump.z*In.normal);
#else
	float3 normal = In.normal;
#endif
	
	float specularInt =
#ifdef SPECULAR_MAP
		tex2D(sampler_specular, In.tex);
#else
		specular_intensity;
#endif
	float specular = 0;
	
	float lightIntensity = 0;
#ifdef POINT_LIGHT
	// oswietlenie punktowe (3 swiatla)
	float3 diffuse = float3(0,0,0);
	for(int i=0; i<LIGHTS; ++i)
	{
		float3 lightVec = normalize(lights[i].pos - In.pos_world);
		float dist = distance(lights[i].pos, In.pos_world);
		float falloff = clamp((1-(dist/lights[i].pos.w)),0,1);
		float light = clamp(dot(lightVec, normal),0,1) * falloff;
		if(light > 0)
		{
			float3 reflection = normalize(light*2*normal - lightVec);
			specular += pow(saturate(dot(reflection, normalize(In.view_dir))), specular_hardness) * specularInt * falloff;
		}							
		diffuse += light * lights[i].color;
		lightIntensity += light;
	}
	lightIntensity = saturate(lightIntensity);
	specular = saturate(specular);
#elif defined(DIR_LIGHT)
	// oswietlenie kierunkowe
	lightIntensity = saturate(dot(normal, light_dir));
	float3 diffuse = light_color * lightIntensity;
	if(lightIntensity > 0)
	{
		float3 reflection = normalize(lightIntensity*2*normal - light_dir);
		specular = pow(saturate(dot(reflection, normalize(In.view_dir))), specular_hardness) * specularInt;
	}
#else
	// brak oswietlenia
	lightIntensity = 1;
	float3 diffuse = float3(1,1,1);
#endif
		
	tex = float4(saturate((tex.xyz * saturate(ambient_color + diffuse) ) + specular_color * specular), tex.w);
	
#ifdef FOG
	return float4(lerp(tex.xyz, fog_color, fog), tex.w);
#else
	return float4(tex.xyz, tex.w);
#endif
}

//******************************************************************************
technique mesh
{
	pass pass0
	{
		VertexShader = compile VS_VERSION vs_mesh();
		PixelShader = compile PS_VERSION ps_mesh();
	}
}
