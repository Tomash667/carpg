#include "Pch.h"
#include "GameCore.h"
#include "OffscreenLocation.h"
#include "Unit.h"

//=================================================================================================
OffscreenLocation::OffscreenLocation() : Location(false), LevelArea(LevelArea::Type::Outside, LevelArea::OUTSIDE_ID, false)
{
}

//=================================================================================================
void OffscreenLocation::Save(GameWriter& f, bool local)
{
	Location::Save(f, local);
	LevelArea::Save(f);
}

//=================================================================================================
void OffscreenLocation::Load(GameReader& f, bool local)
{
	Location::Load(f, local);
	LevelArea::Load(f, local);
}

//=================================================================================================
Unit* OffscreenLocation::CreateUnit(UnitData& data, int level)
{
	Unit* unit = new Unit;
	unit->Init(data, level);
	unit->area = this;
	units.push_back(unit);
	return unit;
}
