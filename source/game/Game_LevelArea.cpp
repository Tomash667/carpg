#include "Pch.h"
#include "Base.h"
#include "Game.h"

ObjectPool<LevelAreaContext> LevelAreaContextPool;

//=================================================================================================
// Get area levels for selected location and level (in multilevel dungeon not generated levels are ignored for -1)
// Level have special meaning here
// >= 0 (dungeon_level, building index)
// -1 whole location
// -2 outside part of city/village
LevelAreaContext* Game::ForLevel(int loc, int level)
{
	LevelAreaContext* lac = LevelAreaContextPool.Get();
	lac->refs = 1;
	lac->entries.clear();

	bool active = (current_location == loc);
	Location* l = locations[loc];
	assert(l->state >= LS_ENTERED);

	switch(l->type)
	{
	case L_CITY:
	case L_VILLAGE:
		{
			City* city = (City*)l;
			if(level == -1)
			{
				lac->entries.resize(city->inside_buildings.size() + 1);
				LevelAreaContext::Entry& e = lac->entries[0];
				e.active = active;
				e.area = city;
				e.level = -2;
				e.loc = loc;
				for(int i = 0, len = (int)city->inside_buildings.size(); i<len; ++i)
				{
					LevelAreaContext::Entry& e2 = lac->entries[i+1];
					e2.active = active;
					e2.area = city->inside_buildings[i];
					e2.level = i;
					e2.loc = loc;
				}
			}
			else if(level == -2)
			{
				LevelAreaContext::Entry& e = Add1(lac->entries);
				e.active = active;
				e.area = city;
				e.level = -2;
				e.loc = loc;
			}
			else
			{
				assert(level >= 0 && level < (int)city->inside_buildings.size());
				LevelAreaContext::Entry& e = Add1(lac->entries);
				e.active = active;
				e.area = city->inside_buildings[level];
				e.level = level;
				e.loc = loc;
			}
		}
		break;
	case L_CAVE:
	case L_DUNGEON:
	case L_CRYPT:
		{
			InsideLocation* inside = (InsideLocation*)l;
			if(inside->IsMultilevel())
			{
				MultiInsideLocation* multi = (MultiInsideLocation*)inside;
				if(level == -1)
				{
					lac->entries.resize(multi->generated);
					for(int i = 0; i<multi->generated; ++i)
					{
						LevelAreaContext::Entry& e = lac->entries[i];
						e.active = (active && dungeon_level == i);
						e.area = &multi->levels[i];
						e.level = i;
						e.loc = loc;
					}
				}
				else
				{
					assert(level >= 0 && level < multi->generated);
					LevelAreaContext::Entry& e = Add1(lac->entries);
					e.active = (active && dungeon_level == level);
					e.area = &multi->levels[level];
					e.level = level;
					e.loc = loc;
				}
			}
			else
			{
				assert(level == -1 || level == 0);
				SingleInsideLocation* single = (SingleInsideLocation*)inside;
				LevelAreaContext::Entry& e = Add1(lac->entries);
				e.active = active;
				e.area = single;
				e.level = 0;
				e.loc = loc;
			}
		}
		break;
	case L_FOREST:
	case L_MOONWELL:
	case L_ENCOUNTER:
	case L_ACADEMY:
	case L_CAMP:
		{
			assert(level == -1);
			OutsideLocation* outside = (OutsideLocation*)l;
			LevelAreaContext::Entry& e = Add1(lac->entries);
			e.active = active;
			e.area = outside;
			e.level = -1;
			e.loc = loc;
		}
		break;
	default:
		assert(0);
		break;
	}

	return lac;
}

//=================================================================================================
GroundItem* Game::FindQuestGroundItem(LevelAreaContext* lac, int quest_refid, LevelAreaContext::Entry*& entry, int& item_index)
{
	assert(lac);

	for(LevelAreaContext::Entry& e : lac->entries)
	{
		for(int i = 0, len = (int)e.area->items.size(); i<len; ++i)
		{
			GroundItem* it = e.area->items[i];
			if(it->item->IsQuest(quest_refid))
			{
				entry = &e;
				item_index = i;
				lac->Free();
				return it;
			}
		}
	}

	lac->Free();
	return NULL;
}

//=================================================================================================
// search only alive enemies for now
Unit* Game::FindUnitWithQuestItem(LevelAreaContext* lac, int quest_refid, LevelAreaContext::Entry*& entry, int& unit_index, int& item_iindex)
{
	assert(lac);

	for(LevelAreaContext::Entry& e : lac->entries)
	{
		for(int i = 0, len = (int)e.area->units.size(); i<len; ++i)
		{
			Unit* unit = e.area->units[i];
			if(unit->IsAlive() && IsEnemy(*unit, *pc->unit))
			{
				int iindex = unit->FindQuestItem(quest_refid);
				if(iindex != INVALID_IINDEX)
				{
					item_iindex = iindex;
					unit_index = i;
					entry = &e;
					lac->Free();
					return unit;
				}
			}
		}
	}

	lac->Free();
	return NULL;
}

//=================================================================================================
bool Game::FindUnit(LevelAreaContext* lac, Unit* unit, LevelAreaContext::Entry*& entry, int& unit_index)
{
	assert(lac && unit);

	for(LevelAreaContext::Entry& e : lac->entries)
	{
		for(int i = 0, len = (int)e.area->units.size(); i<len; ++i)
		{
			Unit* unit2 = e.area->units[i];
			if(unit == unit2)
			{
				entry = &e;
				unit_index = i;
				lac->Free();
				return true;
			}
		}
	}

	lac->Free();
	return false;
}

//=================================================================================================
bool Game::RemoveQuestGroundItem(LevelAreaContext* lac, int quest_refid)
{
	assert(lac);

	lac->AddRef();
	LevelAreaContext::Entry* entry;
	int index;
	GroundItem* item = FindQuestGroundItem(lac, quest_refid, entry, index);
	if(item)
	{
		if(entry->active && IsOnline())
		{
			NetChange& c = Add1(net_changes);
			c.type = NetChange::REMOVE_ITEM;
			c.id = item->netid;
		}
		RemoveElementIndex(entry->area->items, index);
		lac->Free();
		return true;
	}
	else
	{
		lac->Free();
		return false;
	}
}

//=================================================================================================
// search only alive enemies for now
bool Game::RemoveQuestItemFromUnit(LevelAreaContext* lac, int quest_refid)
{
	assert(lac);

	lac->AddRef();
	LevelAreaContext::Entry* entry;
	int unit_index, item_iindex;
	Unit* unit = FindUnitWithQuestItem(lac, quest_refid, entry, unit_index, item_iindex);
	if(unit)
	{
		unit->RemoveItem(item_iindex, entry->active);
		lac->Free();
		return true;
	}
	else
	{
		lac->Free();
		return false;
	}
}

//=================================================================================================
// only remove alive units for now
bool Game::RemoveUnit(LevelAreaContext* lac, Unit* unit)
{
	assert(lac && unit);

	lac->AddRef();
	LevelAreaContext::Entry* entry;
	int unit_index;
	if(FindUnit(lac, unit, entry, unit_index))
	{
		if(entry->active)
		{
			unit->to_remove = true;
			to_remove.push_back(unit);
		}
		else
			RemoveElementIndex(entry->area->units, unit_index);
		lac->Free();
		return true;
	}
	else
	{
		lac->Free();
		return false;
	}
}