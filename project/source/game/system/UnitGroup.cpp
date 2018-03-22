#include "Pch.h"
#include "GameCore.h"
#include "UnitGroup.h"
#include "UnitData.h"

//-----------------------------------------------------------------------------
vector<UnitGroup*> UnitGroup::groups;
vector<UnitGroupList*> UnitGroupList::lists;

//=================================================================================================
UnitGroup* UnitGroup::TryGet(Cstring id)
{
	for(auto group : groups)
	{
		if(group->id == id.s)
			return group;
	}

	return nullptr;
}

//=================================================================================================
UnitGroupList* UnitGroupList::TryGet(Cstring id)
{
	for(auto list : lists)
	{
		if(list->id == id.s)
			return list;
	}

	return nullptr;
}

//=================================================================================================
void TmpUnitGroup::Fill(int level)
{
	assert(group);
	max_level = level;
	total = 0;
	entries.clear();
	for(auto& entry : group->entries)
	{
		if(entry.ud->level.x <= level)
		{
			entries.push_back(entry);
			total += entry.count;
		}
	}
}
