#include "Pch.h"
#include "OffscreenLocation.h"

#include "Unit.h"

//=================================================================================================
OffscreenLocation::OffscreenLocation() : Location(false), LevelArea(LevelArea::Type::Outside, LevelArea::OUTSIDE_ID, false)
{
}

//=================================================================================================
void OffscreenLocation::Save(GameWriter& f)
{
	Location::Save(f);
	LevelArea::Save(f);
}

//=================================================================================================
void OffscreenLocation::Load(GameReader& f)
{
	Location::Load(f);
	LevelArea::Load(f);
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
