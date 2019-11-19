#pragma once

//-----------------------------------------------------------------------------
#include "LevelArea.h"
#include "Building.h"

//-----------------------------------------------------------------------------
struct InsideBuilding final : public LevelArea
{
	Vec2 offset;
	Vec3 inside_spawn, outside_spawn, xsphere_pos;
	Box2d enter_region, exit_region, region1, region2;
	float outside_rot, top, xsphere_radius, enter_y;
	Building* building;
	Int2 level_shift;

	InsideBuilding(int area_id) : LevelArea(LevelArea::Type::Building, area_id, false) {}
	void Save(GameWriter& f);
	void Load(GameReader& f);
	void Write(BitStreamWriter& f);
	bool Read(BitStreamReader& f);
};
