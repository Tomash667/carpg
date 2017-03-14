// wnêtrze budynku
#pragma once

//-----------------------------------------------------------------------------
#include "Location.h"
#include "LevelArea.h"
#include "Building.h"
#include "Blood.h"
#include "Light.h"

//-----------------------------------------------------------------------------
struct InsideBuilding final : public ILevel, public LevelArea
{
	vector<Door*> doors;
	vector<Object> objects;
	vector<Useable*> useables;
	vector<Blood> bloods;
	vector<Light> lights;
	VEC2 offset;
	VEC3 inside_spawn, outside_spawn, xsphere_pos;
	BOX2D enter_area, exit_area, arena1, arena2;
	float outside_rot, top, xsphere_radius, enter_y;
	LevelContext ctx;
	Building* type;
	INT2 level_shift;
	vector<LightMask> masks;

	~InsideBuilding();

	void ApplyContext(LevelContext& ctx);
	void Save(HANDLE file, bool local);
	void Load(HANDLE file, bool local);
	Unit* FindUnit(const UnitData* ud) const;
};
