#include "Pch.h"
#include "LocationHelper.h"

#include "LocationContext.h"
#include "MultiInsideLocation.h"
#include "Quest.h"
#include "ScriptException.h"
#include "SingleInsideLocation.h"

//=================================================================================================
LocationPart* LocationHelper::GetLocationPart(Location* loc)
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
LocationPart* LocationHelper::GetLocationPart(Location* loc, int index)
{
	assert(loc && index >= -1);
	if(index == -1)
		return GetLocationPart(loc);
	if(loc->outside)
	{
		if(loc->type == L_CITY)
		{
			City* city = static_cast<City*>(loc);
			if(index >= 0 && index < (int)city->insideBuildings.size())
				return city->insideBuildings[index];
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
		else if(index == 0)
			return static_cast<SingleInsideLocation*>(loc);
	}
	throw ScriptException("Invalid area index %d.", index);
}

//=================================================================================================
LocationPart* LocationHelper::GetBuildingLocationPart(Location* loc, const string& name)
{
	assert(loc);

	if(loc->type == L_CITY)
	{
		City* city = static_cast<City*>(loc);
		for(InsideBuilding* building : city->insideBuildings)
		{
			if(building->building->group->id == name)
				return building;
		}
	}
	return nullptr;
}

//=================================================================================================
Unit* LocationHelper::GetMayor(Location* loc)
{
	assert(loc);
	if(loc->type != L_CITY)
		return nullptr;
	City* city = static_cast<City*>(loc);
	cstring unit_id;
	if(city->IsVillage())
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
		return unit->questId == questId;
	});
}
