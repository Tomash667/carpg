#pragma once

//-----------------------------------------------------------------------------
enum VertexDeclarationId
{
	VDI_DEFAULT, // Pos Tex Normal
	VDI_ANIMATED, // Pos Weights Indices Tex Normal
	VDI_TANGENT, // Pos Tex Normal Tangent Binormal
	VDI_ANIMATED_TANGENT, // Pos Weights Indices Tex Normal Tangent Binormal
	VDI_TEX, // Pos Tex
	VDI_COLOR, // Pos Color
	VDI_PARTICLE, // Pos Tex Color
	VDI_TERRAIN, // Pos Normal Tex Tex2
	VDI_POS, // Pos
	VDI_GRASS, // Pos Tex Normal Matrix
	VDI_MAX
};

//-----------------------------------------------------------------------------
struct VDefault
{
	VEC3 pos;
	VEC3 normal;
	VEC2 tex;
};

//-----------------------------------------------------------------------------
struct VAnimated
{
	VEC3 pos;
	float weights;
	uint indices;
	VEC3 normal;
	VEC2 tex;
};

//-----------------------------------------------------------------------------
struct VTangent
{
	VTangent() {}
	VTangent(const VEC3& pos, const VEC2& tex, const VEC3& normal, const VEC3& tangent, const VEC3& binormal) : pos(pos), normal(normal), tex(tex),
		tangent(tangent), binormal(binormal) {}

	VEC3 pos;
	VEC3 normal;
	VEC2 tex;
	VEC3 tangent;
	VEC3 binormal;
};

//-----------------------------------------------------------------------------
struct VAnimatedTangent
{
	VEC3 pos;
	float weights;
	uint indices;
	VEC3 normal;
	VEC2 tex;
	VEC3 tangent;
	VEC3 binormal;
};

//-----------------------------------------------------------------------------
struct VTex
{
	VTex() {}
	VTex(float x, float y, float z, float u, float v) : pos(x,y,z), tex(u,v) {}
	VTex(const VEC3& pos, const VEC2& tex) : pos(pos), tex(tex) {}

	VEC3 pos;
	VEC2 tex;
};

//-----------------------------------------------------------------------------
struct VColor
{
	VEC3 pos;
	VEC4 color;
};

//-----------------------------------------------------------------------------
struct VParticle
{
	VEC3 pos;
	VEC2 tex;
	VEC4 color;
};

//-----------------------------------------------------------------------------
struct VTerrain
{
	VTerrain() {}
	VTerrain(float x,float y,float z,float u,float v,float u2,float v2) : pos(x,y,z),tex(u,v),tex2(u2,v2) {}

	VEC3 pos;
	VEC3 normal;
	VEC2 tex;
	VEC2 tex2;

	static const DWORD fvf = D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX2;
};

//-----------------------------------------------------------------------------
struct VPos
{
	VEC3 pos;
};
