// podziemia z wieloma poziomami
#include "Pch.h"
#include "Base.h"
#include "MultiInsideLocation.h"
#include "SaveState.h"

//=================================================================================================
void MultiInsideLocation::ApplyContext(LevelContext& ctx)
{
	ctx.units = &active->units;
	ctx.objects = &active->objects;
	ctx.chests = &active->chests;
	ctx.traps = &active->traps;
	ctx.doors = &active->doors;
	ctx.items = &active->items;
	ctx.useables = &active->useables;
	ctx.bloods = &active->bloods;
	ctx.lights = &active->lights;
	ctx.have_terrain = false;
	ctx.require_tmp_ctx = true;
	ctx.type = LevelContext::Inside;
	ctx.building_id = -1;
	ctx.mine = INT2(0,0);
	ctx.maxe = INT2(active->w,active->h);
	ctx.tmp_ctx = NULL;
	ctx.masks = NULL;
}

//=================================================================================================
void MultiInsideLocation::Save(HANDLE file, bool local)
{
	InsideLocation::Save(file, local);

	WriteFile(file, &active_level, sizeof(active_level), &tmp, NULL);
	WriteFile(file, &generated, sizeof(generated), &tmp, NULL);

	uint ile = levels.size();
	WriteFile(file, &ile, sizeof(ile), &tmp, NULL);
	for(int i=0; i<generated; ++i)
		levels[i].SaveLevel(file, local && i == active_level);

	for(vector<LevelInfo>::iterator it = infos.begin(), end = infos.end(); it != end; ++it)
	{
		WriteFile(file, &it->last_visit, sizeof(it->last_visit), &tmp, NULL);
		WriteFile(file, &it->seed, sizeof(it->seed), &tmp, NULL);
		WriteFile(file, &it->cleared, sizeof(it->cleared), &tmp, NULL);
		WriteFile(file, &it->reset, sizeof(it->reset), &tmp, NULL);
	}
}

//=================================================================================================
void MultiInsideLocation::Load(HANDLE file, bool local)
{
	InsideLocation::Load(file, local);

	ReadFile(file, &active_level, sizeof(active_level), &tmp, NULL);
	ReadFile(file, &generated, sizeof(generated), &tmp, NULL);

	uint ile;
	ReadFile(file, &ile, sizeof(ile), &tmp, NULL);
	levels.resize(ile);
	for(int i=0; i<generated; ++i)
		levels[i].LoadLevel(file, local && active_level == i);

	if(active_level != -1)
		active = &levels[active_level];
	else
		active = NULL;

	infos.resize(ile);
	for(vector<LevelInfo>::iterator it = infos.begin(), end = infos.end(); it != end; ++it)
	{
		ReadFile(file, &it->last_visit, sizeof(it->last_visit), &tmp, NULL);
		if(LOAD_VERSION >= V_0_3)
			ReadFile(file, &it->seed, sizeof(it->seed), &tmp, NULL);
		else
			it->seed = 0;
		ReadFile(file, &it->cleared, sizeof(it->cleared), &tmp, NULL);
		ReadFile(file, &it->reset, sizeof(it->reset), &tmp, NULL);
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
		if(rand2()%3 == 0)
			return 0;
		else
			return 1;
	case 3:
		{
			int a = rand2()%10;
			if(a < 2)
				return 0;
			else if(a < 5)
				return 1;
			else
				return 2;
		}
	case 4:
		{
			int a = rand2()%10;
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
			int a = rand2()%10;
			if(a == 0)
				return levels.size()-4;
			else if(a < 3)
				return levels.size()-3;
			else if(a < 6)
				return levels.size()-2;
			else
				return levels.size()-1;
		}
	}
}

//=================================================================================================
void MultiInsideLocation::BuildRefidTable()
{
	for(vector<InsideLocationLevel>::iterator it = levels.begin(), end = levels.end(); it != end; ++it)
		it->BuildRefidTable();
}

//=================================================================================================
void MultiInsideLocation::RemoveUnit(Unit* unit, int level)
{
	if(level == -1)
	{
		for(int i = 0; i<generated; ++i)
		{
			if(levels[i].RemoveUnit(unit))
				break;
		}
	}
	else if(level < generated)
		levels[level].RemoveUnit(unit);
}

//=================================================================================================
bool MultiInsideLocation::FindUnit(Unit* unit, int* level)
{
	assert(unit);

	for(int i = 0; i<generated; ++i)
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
		for(int i = 0; i<generated; ++i)
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
	
	return NULL;
}

//=================================================================================================
Chest* MultiInsideLocation::FindChestWithItem(const Item* item, int& at_level, int* index)
{
	if(at_level == -1)
	{
		for(int i = 0; i<generated; ++i)
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

	return NULL;
}

//=================================================================================================
Chest* MultiInsideLocation::FindChestWithQuestItem(int quest_refid, int& at_level, int* index)
{
	if(at_level == -1)
	{
		for(int i = 0; i<generated; ++i)
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

	return NULL;
}
