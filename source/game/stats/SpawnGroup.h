#pragma once

//-----------------------------------------------------------------------------
enum SPAWN_GROUP
{
	SG_LOSOWO,
	SG_GOBLINY,
	SG_ORKOWIE,
	SG_BANDYCI,
	SG_NIEUMARLI,
	SG_NEKRO,
	SG_MAGOWIE,
	SG_GOLEMY,
	SG_MAGOWIE_I_GOLEMY,
	SG_ZLO,
	SG_BRAK,
	SG_UNK,
	SG_WYZWANIE,
	SG_MAX
};

//-----------------------------------------------------------------------------
enum KONCOWKA
{
	K_E,
	K_I
};

//-----------------------------------------------------------------------------
struct SpawnGroup
{
	cstring name, id_name, co;
	KONCOWKA k;
	int id, food_mod;
	bool orc_food;
};

//-----------------------------------------------------------------------------
extern SpawnGroup g_spawn_groups[];
