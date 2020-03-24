#include "Pch.h"
#include "SingleInsideLocation.h"

//=================================================================================================
void SingleInsideLocation::Apply(vector<std::reference_wrapper<LevelArea>>& areas)
{
	mine = Int2::Zero;
	maxe = Int2(w, h);
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
void SingleInsideLocation::Load(GameReader& f, bool local)
{
	InsideLocation::Load(f, local);

	if(last_visit != -1)
		LoadLevel(f, local);
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
Chest* SingleInsideLocation::FindChestWithQuestItem(int quest_id, int& at_level, int* index)
{
	Chest* chest = InsideLocationLevel::FindChestWithQuestItem(quest_id, index);
	if(chest)
		at_level = 0;
	return chest;
}
