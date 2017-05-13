#pragma once

//-----------------------------------------------------------------------------
// Materia³ przedmiotu
enum MATERIAL_TYPE
{
	MAT_WOOD,
	MAT_SKIN,
	MAT_IRON,
	MAT_CRYSTAL,
	MAT_CLOTH,
	MAT_ROCK,
	MAT_BODY,
	MAT_BONE
};

/*
enum MATERIAL
{
	M_IRON,
	M_STEEL,
	M_MITHRIL, // mithril mithrilowy
	M_ADAMANTINE, // adamant adamantowy adamantine
	M_WOOD,
	M_ROCK,
	M_SILVER,
	M_GOLD
};

struct Material
{
	MATERIAL id;
	cstring name;
	int str, weigth;
};

extern const Material g_materials[];
extern const uint n_materials;*/

struct MaterialInfo
{
	float intensity;
	int hardness;
};

extern const MaterialInfo g_materials[];
