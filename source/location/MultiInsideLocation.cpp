#include "Pch.h"
#include "MultiInsideLocation.h"

//=================================================================================================
MultiInsideLocation::MultiInsideLocation(int levelCount) : activeLevel(-1), active(nullptr), generated(0)
{
	levels.resize(levelCount);
	for(int i = 0; i < levelCount; ++i)
		levels[i] = new InsideLocationLevel(i);
	LevelInfo li = { -1, false, false, false };
	infos.resize(levelCount, li);
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

	f << activeLevel;
	f << generated;

	const bool prevIsLocal = f.isLocal;
	for(int i = 0; i < generated; ++i)
	{
		f.isLocal = (prevIsLocal && i == activeLevel);
		levels[i]->SaveLevel(f);
	}
	f.isLocal = prevIsLocal;

	for(LevelInfo& info : infos)
	{
		f << info.lastVisit;
		f << info.seed;
		f << info.cleared;
		f << info.reset;
	}
}

//=================================================================================================
void MultiInsideLocation::Load(GameReader& f)
{
	InsideLocation::Load(f);

	f >> activeLevel;
	f >> generated;

	if(LOAD_VERSION < V_0_11)
		f.Read<uint>(); // skip levels count, already set in constructor
	const bool prevIsLocal = f.isLocal;
	for(int i = 0; i < generated; ++i)
	{
		f.isLocal = (prevIsLocal && activeLevel == i);
		levels[i]->LoadLevel(f);
	}
	f.isLocal = prevIsLocal;

	if(activeLevel != -1)
		active = levels[activeLevel];
	else
		active = nullptr;

	infos.resize(levels.size());
	for(LevelInfo& info : infos)
	{
		f >> info.lastVisit;
		f >> info.seed;
		f >> info.cleared;
		f >> info.reset;
		info.loadedResources = false;
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
Chest* MultiInsideLocation::FindChestWithItem(const Item* item, int& atLevel, int* index)
{
	if(atLevel == -1)
	{
		for(int i = 0; i < generated; ++i)
		{
			Chest* chest = levels[i]->FindChestWithItem(item, index);
			if(chest)
			{
				atLevel = i;
				return chest;
			}
		}
	}
	else if(atLevel < generated)
		return levels[atLevel]->FindChestWithItem(item, index);

	return nullptr;
}

//=================================================================================================
Chest* MultiInsideLocation::FindChestWithQuestItem(int questId, int& atLevel, int* index)
{
	if(atLevel == -1)
	{
		for(int i = 0; i < generated; ++i)
		{
			Chest* chest = levels[i]->FindChestWithQuestItem(questId, index);
			if(chest)
			{
				atLevel = i;
				return chest;
			}
		}
	}
	else if(atLevel < generated)
		return levels[atLevel]->FindChestWithQuestItem(questId, index);

	return nullptr;
}

//=================================================================================================
bool MultiInsideLocation::CheckUpdate(int& daysPassed, int worldtime)
{
	bool need_reset = infos[activeLevel].reset;
	infos[activeLevel].reset = false;
	if(infos[activeLevel].lastVisit == -1)
		daysPassed = -1;
	else
		daysPassed = worldtime - infos[activeLevel].lastVisit;
	infos[activeLevel].lastVisit = worldtime;
	return need_reset;
}

//=================================================================================================
bool MultiInsideLocation::LevelCleared()
{
	infos[activeLevel].cleared = true;
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
bool MultiInsideLocation::RequireLoadingResources(bool* toSet)
{
	LevelInfo& info = infos[activeLevel];
	if(toSet)
	{
		bool result = info.loadedResources;
		info.loadedResources = *toSet;
		return result;
	}
	else
		return info.loadedResources;
}
