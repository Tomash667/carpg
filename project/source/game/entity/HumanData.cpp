// dane cz³owieka
#include "Pch.h"
#include "Base.h"
#include "HumanData.h"
#include "Animesh.h"
#include "SaveState.h"
#include "BitStreamFunc.h"

//-----------------------------------------------------------------------------
bool g_beard_and_mustache[MAX_BEARD-1] = {
	false,
	false,
	false,
	false,
	true
};

//-----------------------------------------------------------------------------
#define HEX(h) VEC4(1.f/256*(((h)&0xFF0000)>>16), 1.f/256*(((h)&0xFF00)>>8), 1.f/256*((h)&0xFF), 1.f)
// ze strony http://www.dandwiki.com/wiki/Random_Hair_and_Eye_Color_(DnD_Other)
const VEC4 g_hair_colors[] = {
	HEX(0x000000), // Black
	HEX(0x808080), // Gray
	HEX(0xD3D3D3), // Platinum
	HEX(0xFFFFFF), // White
	HEX(0xB8860B), // Dark Blonde
	HEX(0xDAA520), // Blonde
	HEX(0xF0E68C), // Bleach blonde
	HEX(0x800000), // Dark Redhead
	HEX(0xFF8C00), // Redhead
	HEX(0xF4A460), // Light Redhead
	HEX(0x8B4513), // Brunette
	HEX(0xA0522D)  // Auburn
};
const uint n_hair_colors = countof(g_hair_colors);
// siwy 0xDED5D0

VEC2 Human::GetScale()
{
	float h = (height-1)*0.2f+1.f;
	float w;
	if(height > 1.f)
		w = 1.f+(height-1)*0.4f;
	else if(height < 1.f)
		w = 1.f-(1.f-height)*0.3f;
	else
		w = 1.f;

	return VEC2(w, h);
}

//=================================================================================================
// Ustawienie macierzy na podstawie wysokoœci i wagi
//=================================================================================================
void Human::ApplyScale(Animesh* mesh)
{
	assert(mesh);

	mat_scale.resize(mesh->head.n_bones);
	
	VEC2 scale = GetScale();
	MATRIX m;
	D3DXMatrixScaling(&m, scale.x, scale.y, scale.x);
	for(int i = 0; i<mesh->head.n_bones; ++i)
		mat_scale[i] = m;

	scale.x = (scale.x+1)/2;
	D3DXMatrixScaling(&m, scale.x, scale.y, scale.x);
	mat_scale[4] = m;
	mat_scale[5] = m;
}

//=================================================================================================
void Human::Save(HANDLE file)
{
	WriteFile(file, &hair, sizeof(hair), &tmp, nullptr);
	WriteFile(file, &beard, sizeof(beard), &tmp, nullptr);
	WriteFile(file, &mustache, sizeof(mustache), &tmp, nullptr);
	WriteFile(file, &hair_color, sizeof(hair_color), &tmp, nullptr);
	WriteFile(file, &height, sizeof(height), &tmp, nullptr);
}

//=================================================================================================
void Human::Load(HANDLE file)
{
	ReadFile(file, &hair, sizeof(hair), &tmp, nullptr);
	ReadFile(file, &beard, sizeof(beard), &tmp, nullptr);
	ReadFile(file, &mustache, sizeof(mustache), &tmp, nullptr);
	ReadFile(file, &hair_color, sizeof(hair_color), &tmp, nullptr);
	if(LOAD_VERSION < V_0_2_10)
		ReadFile(file, &height, sizeof(height), &tmp, nullptr); // old weight
	ReadFile(file, &height, sizeof(height), &tmp, nullptr);
}

//=================================================================================================
void HumanData::Get(const Human& h)
{
	hair = h.hair;
	beard = h.beard;
	mustache = h.mustache;
	hair_color = h.hair_color;
	height = h.height;
}

//=================================================================================================
void HumanData::Set(Human& h) const
{
	h.hair = hair;
	h.beard = beard;
	h.mustache = mustache;
	h.hair_color = hair_color;
	h.height = height;
}

//=================================================================================================
void HumanData::CopyFrom(HumanData& hd)
{
	hair = hd.hair;
	beard = hd.beard;
	mustache = hd.mustache;
	hair_color = hd.hair_color;
	height = hd.height;
}

//=================================================================================================
void HumanData::Save(HANDLE file) const
{
	WriteFile(file, &hair, sizeof(hair), &tmp, nullptr);
	WriteFile(file, &beard, sizeof(beard), &tmp, nullptr);
	WriteFile(file, &mustache, sizeof(mustache), &tmp, nullptr);
	WriteFile(file, &hair_color, sizeof(hair_color), &tmp, nullptr);
	WriteFile(file, &height, sizeof(height), &tmp, nullptr);
}

//=================================================================================================
void HumanData::Load(HANDLE file)
{
	ReadFile(file, &hair, sizeof(hair), &tmp, nullptr);
	ReadFile(file, &beard, sizeof(beard), &tmp, nullptr);
	ReadFile(file, &mustache, sizeof(mustache), &tmp, nullptr);
	ReadFile(file, &hair_color, sizeof(hair_color), &tmp, nullptr);
	if(LOAD_VERSION < V_0_2_10)
		ReadFile(file, &height, sizeof(height), &tmp, nullptr); // old weight
	ReadFile(file, &height, sizeof(height), &tmp, nullptr);
}

//=================================================================================================
void HumanData::Write(BitStream& stream) const
{
	stream.WriteCasted<byte>(hair);
	stream.WriteCasted<byte>(beard);
	stream.WriteCasted<byte>(mustache);
	stream.Write(hair_color.x);
	stream.Write(hair_color.y);
	stream.Write(hair_color.z);
	stream.Write(height);
}

//=================================================================================================
int HumanData::Read(BitStream& stream)
{
	if( !stream.ReadCasted<byte>(hair) ||
		!stream.ReadCasted<byte>(beard) ||
		!stream.ReadCasted<byte>(mustache) ||
		!stream.Read(hair_color.x) ||
		!stream.Read(hair_color.y) ||
		!stream.Read(hair_color.z) ||
		!stream.Read(height))
		return 1;

	hair_color.w = 1.f;
	height = clamp(height, 0.9f, 1.1f);

	if(hair == 255)
		hair = -1;
	else if(hair > MAX_HAIR-2)
		return 2;

	if(beard == 255)
		beard = -1;
	else if(beard > MAX_BEARD-2)
		return 2;

	if(mustache == 255)
		mustache = -1;
	else if(mustache > MAX_MUSTACHE-2)
		return 2;

	return 0;
}

//=================================================================================================
void HumanData::Random()
{
	beard = rand2() % MAX_BEARD - 1;
	hair = rand2() % MAX_HAIR - 1;
	mustache = rand2() % MAX_MUSTACHE - 1;
	height = random(0.9f, 1.1f);
	hair_color = g_hair_colors[rand2() % n_hair_colors];
}
