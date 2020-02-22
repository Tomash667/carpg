#include "Pch.h"
#include "LocationHelper.h"
#include "LevelArea.h"

Unit* LocationHelper::GetMayor(Location* loc)
{
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

Unit* LocationHelper::GetCaptain(Location* loc)
{
	if(loc->type != L_CITY)
		return nullptr;
	return ForLocation(loc)->FindUnit(UnitData::Get("guard_captain"));
}
