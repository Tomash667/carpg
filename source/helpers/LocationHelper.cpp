#include "Pch.h"
#include "LocationHelper.h"

#include "LevelArea.h"
#include "MultiInsideLocation.h"
#include "SingleInsideLocation.h"

//=================================================================================================
LevelArea* LocationHelper::GetArea(Location* loc)
{
	assert(loc);
	if(loc->outside)
		return static_cast<OutsideLocation*>(loc);
	InsideLocation* inside = static_cast<InsideLocation*>(loc);
	if(inside->IsMultilevel())
		return static_cast<MultiInsideLocation*>(inside)->levels[0];
	else
		return static_cast<SingleInsideLocation*>(inside);
}

//=================================================================================================
Unit* LocationHelper::GetMayor(Location* loc)
{
	assert(loc);
	if(loc->type != L_CITY)
		return nullptr;
	City* city = static_cast<City*>(loc);
	cstring unit_id;
	if(city->target == VILLAGE)
		unit_id = "soltys";
	else
		unit_id = "mayor";
	return ForLocation(loc)->FindUnit(UnitData::Get(unit_id));
}

//=================================================================================================
Unit* LocationHelper::GetCaptain(Location* loc)
{
	assert(loc);
	if(loc->type != L_CITY)
		return nullptr;
	return ForLocation(loc)->FindUnit(UnitData::Get("guard_captain"));
}
