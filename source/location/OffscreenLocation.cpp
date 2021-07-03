#include "Pch.h"
#include "OffscreenLocation.h"

#include "Unit.h"

//=================================================================================================
OffscreenLocation::OffscreenLocation() : Location(false), LocationPart(LocationPart::Type::Outside, LocationPart::OUTSIDE_ID, false)
{
}

//=================================================================================================
void OffscreenLocation::Save(GameWriter& f)
{
	Location::Save(f);
	LocationPart::Save(f);
}

//=================================================================================================
void OffscreenLocation::Load(GameReader& f)
{
	Location::Load(f);
	LocationPart::Load(f);
}

//=================================================================================================
Unit* OffscreenLocation::CreateUnit(UnitData& data, int level)
{
	Unit* unit = new Unit;
	unit->Init(data, level);
	unit->locPart = this;
	units.push_back(unit);
	return unit;
}
