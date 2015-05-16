// wnêtrze budynku
#pragma once

//-----------------------------------------------------------------------------
#include "Location.h"
#include "Building.h"

//-----------------------------------------------------------------------------
struct InsideBuilding : public ILevel
{
	vector<Unit*> units;
	vector<Door*> doors;
	vector<Object> objects;
	vector<GroundItem*> items;
	vector<Useable*> useables;
	vector<Blood> bloods;
	vector<Light> lights;
	VEC2 offset;
	VEC3 inside_spawn, outside_spawn, xsphere_pos;
	BOX2D enter_area, exit_area, arena1, arena2;
	float outside_rot, top, xsphere_radius, enter_y;
	LevelContext ctx;
	BUILDING type;
	INT2 level_shift;
	vector<LightMask> masks;

	~InsideBuilding();

	void ApplyContext(LevelContext& ctx);
	void Save(HANDLE file, bool local);
	void Load(HANDLE file, bool local);
	Unit* FindUnit(const UnitData* ud) const;
};
