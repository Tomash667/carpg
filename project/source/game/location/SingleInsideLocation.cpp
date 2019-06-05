#include "Pch.h"
#include "GameCore.h"
#include "SingleInsideLocation.h"

//=================================================================================================
void SingleInsideLocation::Apply(vector<std::reference_wrapper<LevelArea>>& areas)
{
	areas.push_back(*this);
}

//=================================================================================================
void SingleInsideLocation::Save(GameWriter& f, bool local)
{
	InsideLocation::Save(f, local);

	if(last_visit != -1)
		SaveLevel(f, local);
}

//=================================================================================================
void SingleInsideLocation::Load(GameReader& f, bool local, LOCATION_TOKEN token)
{
	InsideLocation::Load(f, local, token);

	if(last_visit != -1)
		LoadLevel(f, local);
}

//=================================================================================================
void SingleInsideLocation::BuildRefidTables()
{
	LevelArea::BuildRefidTables();
}

//=================================================================================================
bool SingleInsideLocation::FindUnit(Unit* unit, int* level)
{
	if(HaveUnit(unit))
	{
		if(level)
			*level = 0;
		return true;
	}
	else
		return false;
}

//=================================================================================================
Unit* SingleInsideLocation::FindUnit(UnitData* data, int& at_level)
{
	Unit* u = InsideLocationLevel::FindUnit(data);
	if(u)
		at_level = 0;
	return u;
}

//=================================================================================================
Chest* SingleInsideLocation::FindChestWithItem(const Item* item, int& at_level, int* index)
{
	Chest* chest = InsideLocationLevel::FindChestWithItem(item, index);
	if(chest)
		at_level = 0;
	return chest;
}

//=================================================================================================
Chest* SingleInsideLocation::FindChestWithQuestItem(int quest_refid, int& at_level, int* index)
{
	Chest* chest = InsideLocationLevel::FindChestWithQuestItem(quest_refid, index);
	if(chest)
		at_level = 0;
	return chest;
}
