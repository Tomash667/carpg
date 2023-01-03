#include "Pch.h"
#include "LocationContext.h"

#include "City.h"
#include "Game.h"
#include "Level.h"
#include "MultiInsideLocation.h"
#include "Net.h"
#include "SingleInsideLocation.h"
#include "World.h"

static ObjectPool<LocationContext> LevelAreaContextPool;

//=================================================================================================
// Get location parts for selected location and level (in multilevel dungeon not generated levels are ignored for -1)
// Level have special meaning here
// >= 0 (dungeonLevel, building index)
// -1 whole location
// -2 outside part of city/village
ForLocation::ForLocation(int loc, int level)
{
	Setup(world->GetLocation(loc), level);
}

//=================================================================================================
ForLocation::ForLocation(Location* loc, int level)
{
	Setup(loc, level);
}

//=================================================================================================
void ForLocation::Setup(Location* l, int level)
{
	ctx = LevelAreaContextPool.Get();
	ctx->entries.clear();

	// if level not generated search will find nothing
	if(l->lastVisit == -1)
		return;

	int loc = l->index;
	bool active = (world->GetCurrentLocationIndex() == loc);

	switch(l->type)
	{
	case L_CITY:
		{
			City* city = static_cast<City*>(l);
			if(level == -1)
			{
				ctx->entries.resize(city->insideBuildings.size() + 1);
				LocationContext::Entry& e = ctx->entries[0];
				e.active = active;
				e.locPart = city;
				e.level = -2;
				e.loc = loc;
				for(int i = 0, len = (int)city->insideBuildings.size(); i < len; ++i)
				{
					LocationContext::Entry& e2 = ctx->entries[i + 1];
					e2.active = active;
					e2.locPart = city->insideBuildings[i];
					e2.level = i;
					e2.loc = loc;
				}
			}
			else if(level == -2)
			{
				LocationContext::Entry& e = Add1(ctx->entries);
				e.active = active;
				e.locPart = city;
				e.level = -2;
				e.loc = loc;
			}
			else
			{
				assert(level >= 0 && level < (int)city->insideBuildings.size());
				LocationContext::Entry& e = Add1(ctx->entries);
				e.active = active;
				e.locPart = city->insideBuildings[level];
				e.level = level;
				e.loc = loc;
			}
		}
		break;
	case L_CAVE:
	case L_DUNGEON:
		{
			InsideLocation* inside = static_cast<InsideLocation*>(l);
			if(inside->IsMultilevel())
			{
				MultiInsideLocation* multi = static_cast<MultiInsideLocation*>(inside);
				if(level == -1)
				{
					ctx->entries.resize(multi->generated);
					for(int i = 0; i < multi->generated; ++i)
					{
						LocationContext::Entry& e = ctx->entries[i];
						e.active = (active && gameLevel->dungeonLevel == i);
						e.locPart = multi->levels[i];
						e.level = i;
						e.loc = loc;
					}
				}
				else
				{
					assert(level >= 0 && level < multi->generated);
					LocationContext::Entry& e = Add1(ctx->entries);
					e.active = (active && gameLevel->dungeonLevel == level);
					e.locPart = multi->levels[level];
					e.level = level;
					e.loc = loc;
				}
			}
			else
			{
				assert(level == -1 || level == 0);
				SingleInsideLocation* single = static_cast<SingleInsideLocation*>(inside);
				LocationContext::Entry& e = Add1(ctx->entries);
				e.active = active;
				e.locPart = single;
				e.level = 0;
				e.loc = loc;
			}
		}
		break;
	case L_OUTSIDE:
	case L_ENCOUNTER:
	case L_CAMP:
		{
			assert(level == -1);
			OutsideLocation* outside = static_cast<OutsideLocation*>(l);
			LocationContext::Entry& e = Add1(ctx->entries);
			e.active = active;
			e.locPart = outside;
			e.level = -1;
			e.loc = loc;
		}
		break;
	default:
		assert(0);
		break;
	}
}

//=================================================================================================
ForLocation::~ForLocation()
{
	LevelAreaContextPool.Free(ctx);
}

//=================================================================================================
bool LocationContext::FindUnit(Unit* unit, LocationContext::Entry** entry, int* unitIndex)
{
	assert(unit);

	for(LocationContext::Entry& e : entries)
	{
		for(int i = 0, len = (int)e.locPart->units.size(); i < len; ++i)
		{
			Unit* unit2 = e.locPart->units[i];
			if(unit == unit2)
			{
				if(entry)
					*entry = &e;
				if(unitIndex)
					*unitIndex = i;
				return true;
			}
		}
	}

	return false;
}

//=================================================================================================
Unit* LocationContext::FindUnit(UnitData* data, LocationContext::Entry** entry, int* unitIndex)
{
	assert(data);

	for(LocationContext::Entry& e : entries)
	{
		for(int i = 0, len = (int)e.locPart->units.size(); i < len; ++i)
		{
			Unit* unit = e.locPart->units[i];
			if(unit->data == data)
			{
				if(entry)
					*entry = &e;
				if(unitIndex)
					*unitIndex = i;
				return unit;
			}
		}
	}

	return nullptr;
}

//=================================================================================================
Unit* LocationContext::FindUnit(delegate<bool(Unit*)> clbk, LocationContext::Entry** entry, int* unitIndex)
{
	for(LocationContext::Entry& e : entries)
	{
		for(int i = 0, len = (int)e.locPart->units.size(); i < len; ++i)
		{
			Unit* unit2 = e.locPart->units[i];
			if(clbk(unit2))
			{
				if(entry)
					*entry = &e;
				if(unitIndex)
					*unitIndex = i;
				return unit2;
			}
		}
	}

	return nullptr;
}

//=================================================================================================
// only remove alive units for now
bool LocationContext::RemoveUnit(Unit* unit)
{
	assert(unit);

	LocationContext::Entry* entry;
	int unitIndex;
	if(FindUnit(unit, &entry, &unitIndex))
	{
		if(entry->active)
		{
			unit->toRemove = true;
			gameLevel->toRemove.push_back(unit);
		}
		else
			RemoveElementIndex(entry->locPart->units, unitIndex);
		return true;
	}
	else
		return false;
}
