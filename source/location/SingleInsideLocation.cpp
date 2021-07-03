#include "Pch.h"
#include "SingleInsideLocation.h"

//=================================================================================================
void SingleInsideLocation::Apply(vector<std::reference_wrapper<LocationPart>>& parts)
{
	mine = Int2::Zero;
	maxe = Int2(w, h);
	parts.push_back(*this);
}

//=================================================================================================
void SingleInsideLocation::Save(GameWriter& f)
{
	InsideLocation::Save(f);

	if(last_visit != -1)
		SaveLevel(f);
}

//=================================================================================================
void SingleInsideLocation::Load(GameReader& f)
{
	InsideLocation::Load(f);

	if(last_visit != -1)
		LoadLevel(f);
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
