// podziemia
#include "Pch.h"
#include "Base.h"
#include "InsideLocation.h"

//=================================================================================================
void InsideLocation::Save(HANDLE file, bool local)
{
	Location::Save(file, local);

	WriteFile(file, &target, sizeof(target), &tmp, nullptr);
	WriteFile(file, &special_room, sizeof(special_room), &tmp, nullptr);
	WriteFile(file, &from_portal, sizeof(from_portal), &tmp, nullptr);
}

//=================================================================================================
void InsideLocation::Load(HANDLE file, bool local, LOCATION_TOKEN token)
{
	Location::Load(file, local, token);

	ReadFile(file, &target, sizeof(target), &tmp, nullptr);
	ReadFile(file, &special_room, sizeof(special_room), &tmp, nullptr);
	ReadFile(file, &from_portal, sizeof(from_portal), &tmp, nullptr);
}

//=================================================================================================
bool InsideLocation::RemoveItemFromChest(const Item* item, int& at_level)
{
	assert(item);

	int index;
	Chest* chest = FindChestWithItem(item, at_level, &index);
	if(!chest)
		return false;
	else
	{
		chest->RemoveItem(index);
		return true;
	}
}

//=================================================================================================
bool InsideLocation::RemoveQuestItemFromChest(int quest_refid, int& at_level)
{
	int index;
	Chest* chest = FindChestWithQuestItem(quest_refid, at_level, &index);
	if(!chest)
		return false;
	else
	{
		if(!chest->looted)
			chest->RemoveItem(index);
		return true;
	}
}
