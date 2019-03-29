#include "Pch.h"
#include "GameCore.h"
#include "MultiInsideLocation.h"
#include "SaveState.h"
#include "GameFile.h"

//=================================================================================================
MultiInsideLocation::MultiInsideLocation(int _levels) : active_level(-1), active(nullptr), generated(0)
{
	levels.resize(_levels);
	LevelInfo li = { -1, false, false, false };
	infos.resize(_levels, li);
	for(vector<InsideLocationLevel>::iterator it = levels.begin(), end = levels.end(); it != end; ++it)
		it->map = nullptr;
}

//=================================================================================================
void MultiInsideLocation::ApplyContext(LevelContext& ctx)
{
	ctx.units = &active->units;
	ctx.objects = &active->objects;
	ctx.chests = &active->chests;
	ctx.traps = &active->traps;
	ctx.doors = &active->doors;
	ctx.items = &active->items;
	ctx.usables = &active->usables;
	ctx.bloods = &active->bloods;
	ctx.lights = &active->lights;
	ctx.have_terrain = false;
	ctx.require_tmp_ctx = true;
	ctx.type = LevelContext::Inside;
	ctx.building_id = -1;
	ctx.mine = Int2(0, 0);
	ctx.maxe = Int2(active->w, active->h);
	ctx.tmp_ctx = nullptr;
	ctx.masks = nullptr;
}

//=================================================================================================
void MultiInsideLocation::Save(GameWriter& f, bool local)
{
	InsideLocation::Save(f, local);

	f << active_level;
	f << generated;

	f << levels.size();
	for(int i = 0; i < generated; ++i)
		levels[i].SaveLevel(f, local && i == active_level);

	for(LevelInfo& info : infos)
	{
		f << info.last_visit;
		f << info.seed;
		f << info.cleared;
		f << info.reset;
	}
}

//=================================================================================================
void MultiInsideLocation::Load(GameReader& f, bool local, LOCATION_TOKEN token)
{
	InsideLocation::Load(f, local, token);
	
	f >> active_level;
	f >> generated;

	uint count = f.Read<uint>();
	levels.resize(count);
	for(int i = 0; i < generated; ++i)
		levels[i].LoadLevel(f, local && active_level == i);

	if(active_level != -1)
		active = &levels[active_level];
	else
		active = nullptr;

	infos.resize(count);
	for(LevelInfo& info : infos)
	{
		f >> info.last_visit;
		f >> info.seed;
		f >> info.cleared;
		f >> info.reset;
		info.loaded_resources = false;
	}
}

//=================================================================================================
/*
1 - 100%
1 - 33% 2 - 66%
1 - 20% 2 - 30% 3 - 50%
1 - 10% 2 - 20% 3 - 30% 4 - 40%
*/
int MultiInsideLocation::GetRandomLevel() const
{
	switch(levels.size())
	{
	case 1:
		return 0;
	case 2:
		if(Rand() % 3 == 0)
			return 0;
		else
			return 1;
	case 3:
		{
			int a = Rand() % 10;
			if(a < 2)
				return 0;
			else if(a < 5)
				return 1;
			else
				return 2;
		}
	case 4:
		{
			int a = Rand() % 10;
			if(a == 0)
				return 0;
			else if(a < 3)
				return 1;
			else if(a < 6)
				return 2;
			else
				return 3;
		}
	default:
		{
			int a = Rand() % 10;
			if(a == 0)
				return levels.size() - 4;
			else if(a < 3)
				return levels.size() - 3;
			else if(a < 6)
				return levels.size() - 2;
			else
				return levels.size() - 1;
		}
	}
}

//=================================================================================================
void MultiInsideLocation::BuildRefidTables()
{
	for(vector<InsideLocationLevel>::iterator it = levels.begin(), end = levels.end(); it != end; ++it)
		it->BuildRefidTables();
}

//=================================================================================================
bool MultiInsideLocation::FindUnit(Unit* unit, int* level)
{
	assert(unit);

	for(int i = 0; i < generated; ++i)
	{
		if(levels[i].FindUnit(unit))
		{
			if(level)
				*level = i;
			return true;
		}
	}

	return false;
}

//=================================================================================================
Unit* MultiInsideLocation::FindUnit(UnitData* data, int& at_level)
{
	if(at_level == -1)
	{
		for(int i = 0; i < generated; ++i)
		{
			Unit* u = levels[i].FindUnit(data);
			if(u)
			{
				at_level = i;
				return u;
			}
		}
	}
	else if(at_level < generated)
		return levels[at_level].FindUnit(data);

	return nullptr;
}

//=================================================================================================
Chest* MultiInsideLocation::FindChestWithItem(const Item* item, int& at_level, int* index)
{
	if(at_level == -1)
	{
		for(int i = 0; i < generated; ++i)
		{
			Chest* chest = levels[i].FindChestWithItem(item, index);
			if(chest)
			{
				at_level = i;
				return chest;
			}
		}
	}
	else if(at_level < generated)
		return levels[at_level].FindChestWithItem(item, index);

	return nullptr;
}

//=================================================================================================
Chest* MultiInsideLocation::FindChestWithQuestItem(int quest_refid, int& at_level, int* index)
{
	if(at_level == -1)
	{
		for(int i = 0; i < generated; ++i)
		{
			Chest* chest = levels[i].FindChestWithQuestItem(quest_refid, index);
			if(chest)
			{
				at_level = i;
				return chest;
			}
		}
	}
	else if(at_level < generated)
		return levels[at_level].FindChestWithQuestItem(quest_refid, index);

	return nullptr;
}

//=================================================================================================
bool MultiInsideLocation::CheckUpdate(int& days_passed, int worldtime)
{
	bool need_reset = infos[active_level].reset;
	infos[active_level].reset = false;
	if(infos[active_level].last_visit == -1)
		days_passed = -1;
	else
		days_passed = worldtime - infos[active_level].last_visit;
	infos[active_level].last_visit = worldtime;
	return need_reset;
}

//=================================================================================================
bool MultiInsideLocation::LevelCleared()
{
	infos[active_level].cleared = true;
	for(vector<LevelInfo>::iterator it = infos.begin(), end = infos.end(); it != end; ++it)
	{
		if(!it->cleared)
			return false;
	}
	return true;
}

//=================================================================================================
void MultiInsideLocation::Reset()
{
	for(vector<LevelInfo>::iterator it = infos.begin(), end = infos.end(); it != end; ++it)
	{
		it->reset = true;
		it->cleared = false;
	}
}

//=================================================================================================
bool MultiInsideLocation::RequireLoadingResources(bool* to_set)
{
	LevelInfo& info = infos[active_level];
	if(to_set)
	{
		bool result = info.loaded_resources;
		info.loaded_resources = *to_set;
		return result;
	}
	else
		return info.loaded_resources;
}
