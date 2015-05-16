//******************************************************************************
// Zmienne
//******************************************************************************
uniform texture t0;
uniform float power;
uniform float time;
uniform float4 skill;

//******************************************************************************
// Tekstury
//******************************************************************************
sampler texture0 = sampler_state
{
	Texture = <t0>;
	MinFilter = Linear;
	MagFilter = Linear;
	AddressU = Clamp;
	AddressV = Clamp;
};

//******************************************************************************
// Vertex shader który nic nie robi
//******************************************************************************
void Empty_VS(float3 pos:POSITION,float2 tex:TEXCOORD0,out float4 opos:POSITION,out float2 otex:TEXCOORD0)
{
	opos = float4(pos,1);
	otex = tex;
}
float4 Empty_PS(in float2 tex:TEXCOORD0) : COLOR
{
	return tex2D(texture0, tex);
}
technique Empty
{
	pass pass0
	{
		VertexShader = compile VS_VERSION Empty_VS();
		PixelShader = compile PS_VERSION Empty_PS();
	}
}

//******************************************************************************
// Przekszta³ca na szary obraz, parametry:
// float power(0..1) - 0 brak efektu, 1 ca³kowicie szary
//******************************************************************************
float4 Monochrome_PS(float2 tex:TEXCOORD0) : COLOR
{
	float4 base_color = tex2D(texture0,tex);
	float4 color;
	color.a = base_color.a;
	color.rgb = (base_color.r+base_color.g+base_color.b)/3.0f;
	return lerp(base_color,color,power);
}
technique Monochrome
{
	pass pass0
	{
		VertexShader = compile VS_VERSION Empty_VS();
		PixelShader = compile PS_VERSION Monochrome_PS();
	}
}

//******************************************************************************
// Efekt snu, parametry:
// float time - postêp efektu
// float power - si³a (0..1)
// float4 skill - gdzie:
//   0 - blur (2)
//   1 - sharpness (7)
//   2 - speed (0.1)
//   3 - range (0.4)
//******************************************************************************
float4 Dream_PS(float2 tex:TEXCOORD0) : COLOR
{	
	float4 base_color = tex2D(texture0,tex);
	tex.xy -= 0.5;
	tex.xy *= 1-(sin(time*skill[2])*skill[3]+skill[3])*0.5;
	tex.xy += 0.5;
	
	float4 color = tex2D( texture0, tex.xy);

	color += tex2D( texture0, tex.xy+0.001*skill[0]);
	color += tex2D( texture0, tex.xy+0.003*skill[0]);
	color += tex2D( texture0, tex.xy+0.005*skill[0]);
	color += tex2D( texture0, tex.xy+0.007*skill[0]);
	color += tex2D( texture0, tex.xy+0.009*skill[0]);
	color += tex2D( texture0, tex.xy+0.011*skill[0]);

	color += tex2D( texture0, tex.xy-0.001*skill[0]);
	color += tex2D( texture0, tex.xy-0.003*skill[0]);
	color += tex2D( texture0, tex.xy-0.005*skill[0]);
	color += tex2D( texture0, tex.xy-0.007*skill[0]);
	color += tex2D( texture0, tex.xy-0.009*skill[0]);
	color += tex2D( texture0, tex.xy-0.011*skill[0]);

	color.rgb = (color.r+color.g+color.b)/3.0f;

	color /= skill[1];
	return lerp(base_color,color,power);
}
technique Dream
{
	pass pass0
	{
		VertexShader = compile VS_VERSION Empty_VS();
		PixelShader = compile PS_VERSION Dream_PS();
	}
}

//******************************************************************************
// Efekt rozmycia, trzeba robiæ 2 razy, raz po x, raz po y, parametry:
// float power - si³a (0..1)
// float4 skill - gdzie:
// 	0 - odstêp po x
// 	1 - odstêp po y
//******************************************************************************
float4 BlurX_PS(float2 tex:TEXCOORD0) : COLOR
{
	float4
	c  = tex2D(texture0, float2(tex.x - 4.f*skill[0], tex.y)) * 0.05f;
	c += tex2D(texture0, float2(tex.x - 3.f*skill[0], tex.y)) * 0.09f;
	c += tex2D(texture0, float2(tex.x - 2.f*skill[0], tex.y)) * 0.12f;
	c += tex2D(texture0, float2(tex.x - 1.f*skill[0], tex.y)) * 0.15f;
	c += tex2D(texture0, float2(tex.x               , tex.y)) * 0.16f;
	c += tex2D(texture0, float2(tex.x + 1.f*skill[0], tex.y)) * 0.15f;
	c += tex2D(texture0, float2(tex.x + 2.f*skill[0], tex.y)) * 0.12f;
	c += tex2D(texture0, float2(tex.x + 3.f*skill[0], tex.y)) * 0.09f;
	c += tex2D(texture0, float2(tex.x + 4.f*skill[0], tex.y)) * 0.05f;
	return lerp(tex2D(texture0, tex), c, power);
}
technique BlurX
{
	pass pass0
	{
		VertexShader = compile VS_VERSION Empty_VS();
		PixelShader = compile PS_VERSION BlurX_PS();
	}
}
float4 BlurY_PS(float2 tex:TEXCOORD0) : COLOR
{
	float4
	c  = tex2D(texture0, float2(tex.x, tex.y - 4.f*skill[1])) * 0.05f;
	c += tex2D(texture0, float2(tex.x, tex.y - 3.f*skill[1])) * 0.09f;
	c += tex2D(texture0, float2(tex.x, tex.y - 2.f*skill[1])) * 0.12f;
	c += tex2D(texture0, float2(tex.x, tex.y - 1.f*skill[1])) * 0.15f;
	c += tex2D(texture0, float2(tex.x, tex.y               )) * 0.16f;
	c += tex2D(texture0, float2(tex.x, tex.y + 1.f*skill[1])) * 0.15f;
	c += tex2D(texture0, float2(tex.x, tex.y + 2.f*skill[1])) * 0.12f;
	c += tex2D(texture0, float2(tex.x, tex.y + 3.f*skill[1])) * 0.09f;
	c += tex2D(texture0, float2(tex.x, tex.y + 4.f*skill[1])) * 0.05f;
	return lerp(tex2D(texture0, tex), c, power);
}
technique BlurY
{
	pass pass0
	{
		VertexShader = compile VS_VERSION Empty_VS();
		PixelShader = compile PS_VERSION BlurY_PS();
	}
}
