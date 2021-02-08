#include "Pch.h"
#include "LocationHelper.h"

#include "LevelAreaContext.h"
#include "MultiInsideLocation.h"
#include "Quest.h"
#include "ScriptException.h"
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
LevelArea* LocationHelper::GetArea(Location* loc, int index)
{
	assert(loc && index >= -1);
	if(index == -1)
		return GetArea(loc);
	if(loc->outside)
	{
		if(loc->type == L_CITY)
		{
			City* city = static_cast<City*>(loc);
			if(index >= 0 && index < (int)city->inside_buildings.size())
				return city->inside_buildings[index];
		}
	}
	else
	{
		InsideLocation* inside = static_cast<InsideLocation*>(loc);
		if(inside->IsMultilevel())
		{
			MultiInsideLocation* multi = static_cast<MultiInsideLocation*>(inside);
			if(index >= 0 && index < (int)multi->levels.size())
				return multi->levels[index];
		}
	}
	throw ScriptException("Invalid area index %d.", index);
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

//=================================================================================================
Unit* LocationHelper::FindQuestUnit(Location* loc, Quest* quest)
{
	assert(loc && quest);
	const int questId = quest->id;
	return ForLocation(loc)->FindUnit([=](Unit* unit)
	{
		return unit->quest_id == questId;
	});
}
