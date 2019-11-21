// dane cz³owieka
#include "Pch.h"
#include "GameCore.h"
#include "HumanData.h"
#include "MeshInstance.h"
#include "SaveState.h"
#include "BitStreamFunc.h"

//-----------------------------------------------------------------------------
bool g_beard_and_mustache[MAX_BEARD - 1] = {
	false,
	false,
	false,
	false,
	true
};

//-----------------------------------------------------------------------------
// ze strony http://www.dandwiki.com/wiki/Random_Hair_and_Eye_Color_(DnD_Other)
const Vec4 g_hair_colors[] = {
	Color::Hex(0x000000), // Black
	Color::Hex(0x808080), // Gray
	Color::Hex(0xD3D3D3), // Platinum
	Color::Hex(0xFFFFFF), // White
	Color::Hex(0xB8860B), // Dark Blonde
	Color::Hex(0xDAA520), // Blonde
	Color::Hex(0xF0E68C), // Bleach blonde
	Color::Hex(0x800000), // Dark Redhead
	Color::Hex(0xFF8C00), // Redhead
	Color::Hex(0xF4A460), // Light Redhead
	Color::Hex(0x8B4513), // Brunette
	Color::Hex(0xA0522D)  // Auburn
};
const uint n_hair_colors = countof(g_hair_colors);
// siwy 0xDED5D0

Vec2 Human::GetScale()
{
	float h = (height - 1)*0.2f + 1.f;
	float w;
	if(height > 1.f)
		w = 1.f + (height - 1)*0.4f;
	else if(height < 1.f)
		w = 1.f - (1.f - height)*0.3f;
	else
		w = 1.f;

	return Vec2(w, h);
}

//=================================================================================================
// Ustawienie macierzy na podstawie wysokoœci i wagi
//=================================================================================================
void Human::ApplyScale(MeshInstance* mesh_inst)
{
	assert(mesh_inst);

	mat_scale.resize(mesh_inst->mesh->head.n_bones);
	mesh_inst->mat_scale = mat_scale.data();

	Vec2 scale = GetScale();
	Matrix m = Matrix::Scale(scale.x, scale.y, scale.x);
	for(int i = 0; i < mesh_inst->mesh->head.n_bones; ++i)
		mat_scale[i] = m;

	scale.x = (scale.x + 1) / 2;
	m = Matrix::Scale(scale.x, scale.y, scale.x);
	mat_scale[4] = m;
	mat_scale[5] = m;
}

//=================================================================================================
void Human::Save(FileWriter& f)
{
	f << hair;
	f << beard;
	f << mustache;
	f << hair_color;
	f << height;
}

//=================================================================================================
void Human::Load(FileReader& f)
{
	f >> hair;
	f >> beard;
	f >> mustache;
	f >> hair_color;
	f >> height;
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
void HumanData::Save(FileWriter& f) const
{
	f << hair;
	f << beard;
	f << mustache;
	f << hair_color;
	f << height;
}

//=================================================================================================
void HumanData::Load(FileReader& f)
{
	f >> hair;
	f >> beard;
	f >> mustache;
	f >> hair_color;
	f >> height;
}

//=================================================================================================
void HumanData::Write(BitStreamWriter& f) const
{
	f.WriteCasted<byte>(hair);
	f.WriteCasted<byte>(beard);
	f.WriteCasted<byte>(mustache);
	f << hair_color.x;
	f << hair_color.y;
	f << hair_color.z;
	f << height;
}

//=================================================================================================
int HumanData::Read(BitStreamReader& f)
{
	f.ReadCasted<byte>(hair);
	f.ReadCasted<byte>(beard);
	f.ReadCasted<byte>(mustache);
	f >> hair_color.x;
	f >> hair_color.y;
	f >> hair_color.z;
	f >> height;
	if(!f)
		return 1;

	hair_color.w = 1.f;
	height = Clamp(height, 0.9f, 1.1f);

	if(hair == 255)
		hair = -1;
	else if(hair > MAX_HAIR - 2)
		return 2;

	if(beard == 255)
		beard = -1;
	else if(beard > MAX_BEARD - 2)
		return 2;

	if(mustache == 255)
		mustache = -1;
	else if(mustache > MAX_MUSTACHE - 2)
		return 2;

	return 0;
}

//=================================================================================================
void HumanData::Random()
{
	beard = Rand() % MAX_BEARD - 1;
	hair = Rand() % MAX_HAIR - 1;
	mustache = Rand() % MAX_MUSTACHE - 1;
	height = ::Random(0.9f, 1.1f);
	hair_color = g_hair_colors[Rand() % n_hair_colors];
}
