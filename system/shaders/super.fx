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
shared float4x4 matCombined;
shared float4x4 matWorld;
shared float4x4 matBones[32];

// dodatkowy kolor/alpha
shared float4 tint;

// oswietlenie ambient
shared float3 ambientColor;

// mgła
shared float4 fogColor;
shared float4 fogParams; //(x=fogStart,y=fogEnd,z=(fogEnd-fogStart),a=?)

// oswietlenie na zewnatrz
shared float3 lightDir;
shared float3 lightColor;

// oswietlenie wewnatrz
struct Light
{
	float4 pos;
	float3 color;	
};
shared Light lights[3];

shared float3 cameraPos;
shared float3 specularColor;
shared float specularIntensity;
shared float specularHardness;

// tekstury
shared texture texDiffuse;
shared texture texNormal;
shared texture texSpecular;

// samplery
sampler2D samplerDiffuse = sampler_state
{
	texture = <texDiffuse>;
	MipFilter = Linear;
	MinFilter = Linear;
	MagFilter = Linear;
};
sampler2D samplerNormal = sampler_state
{
	texture = <texNormal>;
	MipFilter = Linear;
	MinFilter = Linear;
	MagFilter = Linear;
};
sampler2D samplerSpecular = sampler_state
{
	texture = <texSpecular>;
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
		float posViewZ : TEXCOORD2;
	#endif
	#ifdef POINT_LIGHT
		float3 posWorld : TEXCOORD3;
	#endif
	#ifdef NORMAL_MAP
		float3 tangent : TEXCOORD4;
		float3 binormal : TEXCOORD5;
	#endif
	float3 viewDir : TEXCOORD6;
};

//******************************************************************************
void vs_mesh(in MESH_INPUT In, out MESH_OUTPUT Out)
{
	// uv tekstury
	Out.tex = In.tex;
	
	// pozycja
	#ifdef ANIMATED
		float3 pos = mul(float4(In.pos,1), matBones[In.indices[0]]) * In.weight;
		pos += mul(float4(In.pos,1), matBones[In.indices[1]]) * (1-In.weight);
		Out.pos = mul(float4(pos,1), matCombined);
	#else
		float3 pos = In.pos;
		Out.pos = mul(float4(In.pos,1), matCombined);
	#endif

	// normalna / kolor
	#if defined(ANIMATED)
		float3 normal = mul(float4(In.normal,1), matBones[In.indices[0]]) * In.weight;
		normal += mul(float4(In.normal,1), matBones[In.indices[1]]) * (1-In.weight);
		Out.normal = mul(normal, matWorld);
	#else
		Out.normal = mul(In.normal, matWorld);
	#endif

	// odleglosc dla mgly
	#ifdef FOG
		Out.posViewZ = Out.pos.w;
	#endif
	
	// pozycja w odniesieniu do pozycji swiata
	#ifdef POINT_LIGHT
		Out.posWorld = mul(float4(pos,1), matWorld);
	#endif
	
	#ifdef NORMAL_MAP
		Out.tangent = normalize(mul(In.tangent,matWorld));
		Out.binormal = normalize(mul(In.binormal,matWorld));
	#endif

	#ifdef POINT_LIGHT
		Out.viewDir = cameraPos - Out.posWorld;
	#else
		Out.viewDir = cameraPos - mul(float4(pos,1), matWorld);
	#endif
}

//******************************************************************************
float4 ps_mesh(in MESH_OUTPUT In) : COLOR0
{
	float4 tex = tex2D(samplerDiffuse, In.tex) * tint;
	
	#if defined(FOG) //&& !defined(POINT_LIGHT)
		float fog = saturate((In.posViewZ-fogParams.x)/fogParams.z);
	#endif

	#if defined(NORMAL_MAP)
		float3 bump = tex2D(samplerNormal, In.tex) * 2.f - 1.f;
		float3 normal = normalize(bump.x*In.tangent + (-bump.y)*In.binormal + bump.z*In.normal);
	#else
		float3 normal = In.normal;
	#endif
	//return float4(normal, 1);
	
	float specularInt =
	#ifdef SPECULAR_MAP
		tex2D(samplerSpecular, In.tex);
	#else
		specularIntensity;
	#endif
	float specular = 0;
	
	float lightIntensity = 0;
	#ifdef POINT_LIGHT
		// oswietlenie punktowe (3 swiatla)
		float3 diffuse = float3(0,0,0);
		for(int i=0; i<LIGHTS; ++i)
		{
			float3 lightVec = normalize(lights[i].pos - In.posWorld);
			float dist = distance(lights[i].pos, In.posWorld);
			float falloff = clamp((1-(dist/lights[i].pos.w)),0,1);
			//return falloff;
			float light = clamp(dot(lightVec, normal),0,1) * falloff;
			//return float4(light, light, light, 1);
			if(light > 0)
			{
				float3 reflection = normalize(light*2*normal - lightVec);
				specular += pow(saturate(dot(reflection, normalize(In.viewDir))), specularHardness) * specularInt * falloff;
			}							
			diffuse += light * lights[i].color;
			lightIntensity += light;
		}
		lightIntensity = saturate(lightIntensity);
		specular = saturate(specular);
		//float fog = min(saturate((In.posViewZ-fogParams.x)/fogParams.z), 1-min(lightIntensity, 1));
	#elif defined(DIR_LIGHT)
		// oswietlenie kierunkowe
		lightIntensity = saturate(dot(normal, lightDir));
		float3 diffuse = lightColor * lightIntensity;
		if(lightIntensity > 0)
		{
			float3 reflection = normalize(lightIntensity*2*normal - lightDir);
			specular = pow(saturate(dot(reflection, normalize(In.viewDir))), specularHardness) * specularInt;
		}
	#else
		// brak oswietlenia
		lightIntensity = 1;
		float3 diffuse = float3(1,1,1);
	#endif
		
	//return float4(lightIntensity, lightIntensity, lightIntensity, 1);
	//return specular;			
	
	tex = float4(saturate((tex.xyz * saturate(ambientColor + diffuse) ) + specularColor * specular), tex.w);
	
	#ifdef FOG
		return float4(lerp(tex.xyz, fogColor, fog), tex.w);
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
