#include "Pch.h"
#include "MultiInsideLocation.h"

//=================================================================================================
MultiInsideLocation::MultiInsideLocation(int level_count) : active_level(-1), active(nullptr), generated(0)
{
	levels.resize(level_count);
	for(int i = 0; i < level_count; ++i)
		levels[i] = new InsideLocationLevel(i);
	LevelInfo li = { -1, false, false, false };
	infos.resize(level_count, li);
}

//=================================================================================================
MultiInsideLocation::~MultiInsideLocation()
{
	DeleteElements(levels);
}

//=================================================================================================
void MultiInsideLocation::Apply(vector<std::reference_wrapper<LocationPart>>& parts)
{
	active->mine = Int2::Zero;
	active->maxe = Int2(active->w, active->h);
	parts.push_back(*active);
}

//=================================================================================================
void MultiInsideLocation::Save(GameWriter& f)
{
	InsideLocation::Save(f);

	f << active_level;
	f << generated;

	const bool prevIsLocal = f.isLocal;
	for(int i = 0; i < generated; ++i)
	{
		f.isLocal = (prevIsLocal && i == active_level);
		levels[i]->SaveLevel(f);
	}
	f.isLocal = prevIsLocal;

	for(LevelInfo& info : infos)
	{
		f << info.last_visit;
		f << info.seed;
		f << info.cleared;
		f << info.reset;
	}
}

//=================================================================================================
void MultiInsideLocation::Load(GameReader& f)
{
	InsideLocation::Load(f);

	f >> active_level;
	f >> generated;

	if(LOAD_VERSION < V_0_11)
		f.Read<uint>(); // skip levels count, already set in constructor
	const bool prevIsLocal = f.isLocal;
	for(int i = 0; i < generated; ++i)
	{
		f.isLocal = (prevIsLocal && active_level == i);
		levels[i]->LoadLevel(f);
	}
	f.isLocal = prevIsLocal;

	if(active_level != -1)
		active = levels[active_level];
	else
		active = nullptr;

	infos.resize(levels.size());
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
Chest* MultiInsideLocation::FindChestWithItem(const Item* item, int& at_level, int* index)
{
	if(at_level == -1)
	{
		for(int i = 0; i < generated; ++i)
		{
			Chest* chest = levels[i]->FindChestWithItem(item, index);
			if(chest)
			{
				at_level = i;
				return chest;
			}
		}
	}
	else if(at_level < generated)
		return levels[at_level]->FindChestWithItem(item, index);

	return nullptr;
}

//=================================================================================================
Chest* MultiInsideLocation::FindChestWithQuestItem(int quest_id, int& at_level, int* index)
{
	if(at_level == -1)
	{
		for(int i = 0; i < generated; ++i)
		{
			Chest* chest = levels[i]->FindChestWithQuestItem(quest_id, index);
			if(chest)
			{
				at_level = i;
				return chest;
			}
		}
	}
	else if(at_level < generated)
		return levels[at_level]->FindChestWithQuestItem(quest_id, index);

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
