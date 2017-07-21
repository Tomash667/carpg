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
	Vec3 pos;
	Vec3 normal;
	Vec2 tex;
};

//-----------------------------------------------------------------------------
struct VAnimated
{
	Vec3 pos;
	float weights;
	uint indices;
	Vec3 normal;
	Vec2 tex;
};

//-----------------------------------------------------------------------------
struct VTangent
{
	VTangent() {}
	VTangent(const Vec3& pos, const Vec2& tex, const Vec3& normal, const Vec3& tangent, const Vec3& binormal) : pos(pos), normal(normal), tex(tex),
		tangent(tangent), binormal(binormal) {}

	Vec3 pos;
	Vec3 normal;
	Vec2 tex;
	Vec3 tangent;
	Vec3 binormal;
};

//-----------------------------------------------------------------------------
struct VAnimatedTangent
{
	Vec3 pos;
	float weights;
	uint indices;
	Vec3 normal;
	Vec2 tex;
	Vec3 tangent;
	Vec3 binormal;
};

//-----------------------------------------------------------------------------
struct VTex
{
	VTex() {}
	VTex(float x, float y, float z, float u, float v) : pos(x, y, z), tex(u, v) {}
	VTex(const Vec3& pos, const Vec2& tex) : pos(pos), tex(tex) {}

	Vec3 pos;
	Vec2 tex;
};

//-----------------------------------------------------------------------------
struct VColor
{
	Vec3 pos;
	Vec4 color;
};

//-----------------------------------------------------------------------------
struct VParticle
{
	Vec3 pos;
	Vec2 tex;
	Vec4 color;
};

//-----------------------------------------------------------------------------
struct VTerrain
{
	VTerrain() {}
	VTerrain(float x, float y, float z, float u, float v, float u2, float v2) : pos(x, y, z), tex(u, v), tex2(u2, v2) {}

	Vec3 pos;
	Vec3 normal;
	Vec2 tex;
	Vec2 tex2;

	static const DWORD fvf = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX2;
};

//-----------------------------------------------------------------------------
struct VPos
{
	Vec3 pos;
};
