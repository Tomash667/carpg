#include "Pch.h"
#include "GameCore.h"
#include "InsideLocation.h"
#include "GameFile.h"

//=================================================================================================
void InsideLocation::Save(GameWriter& f, bool local)
{
	Location::Save(f, local);

	f << target;
	f << special_room;
	f << from_portal;
}

//=================================================================================================
void InsideLocation::Load(GameReader& f, bool local, LOCATION_TOKEN token)
{
	Location::Load(f, local, token);

	f >> target;
	f >> special_room;
	f >> from_portal;
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
