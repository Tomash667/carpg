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
struct UnitData;
struct UnitGroup;

//-----------------------------------------------------------------------------
struct SpawnGroup
{
	cstring id, unit_group_id, name;
	UnitGroup* unit_group;
	KONCOWKA k;
	int food_mod;
	bool orc_food;

	inline SpawnGroup(cstring id, cstring unit_group_id, cstring name, KONCOWKA k, int food_mod, bool orc_food) : id(id), unit_group_id(unit_group_id),
		name(name), k(k), unit_group(nullptr), food_mod(food_mod), orc_food(orc_food)
	{
	}

	UnitData* GetSpawnLeader() const;
};

//-----------------------------------------------------------------------------
extern SpawnGroup g_spawn_groups[];
