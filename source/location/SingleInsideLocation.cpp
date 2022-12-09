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

	if(lastVisit != -1)
		SaveLevel(f);
}

//=================================================================================================
void SingleInsideLocation::Load(GameReader& f)
{
	InsideLocation::Load(f);

	if(lastVisit != -1)
		LoadLevel(f);
}

//=================================================================================================
Chest* SingleInsideLocation::FindChestWithItem(const Item* item, int& atLevel, int* index)
{
	Chest* chest = InsideLocationLevel::FindChestWithItem(item, index);
	if(chest)
		atLevel = 0;
	return chest;
}

//=================================================================================================
Chest* SingleInsideLocation::FindChestWithQuestItem(int questId, int& atLevel, int* index)
{
	Chest* chest = InsideLocationLevel::FindChestWithQuestItem(questId, index);
	if(chest)
		atLevel = 0;
	return chest;
}
