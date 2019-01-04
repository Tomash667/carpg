#include "Pch.h"
#include "GameCore.h"
#include "UnitGroup.h"
#include "UnitData.h"

//-----------------------------------------------------------------------------
vector<UnitGroup*> UnitGroup::groups;
vector<UnitGroupList*> UnitGroupList::lists;

//=================================================================================================
bool UnitGroup::HaveLeader() const
{
	for(const UnitGroup::Entry& entry : entries)
	{
		if(entry.is_leader)
			return true;
	}
	return false;
}

//=================================================================================================
UnitData* UnitGroup::GetLeader(int level) const
{
	int best = -1, best_dif, index = 0;
	for(const UnitGroup::Entry& entry : entries)
	{
		if(entry.is_leader)
		{
			int dif = entry.ud->GetLevelDif(level);
			if(best == -1 || best_dif > dif)
			{
				best = index;
				best_dif = dif;
			}
		}
		++index;
	}
	return best == -1 ? nullptr : entries[best].ud;
}

//=================================================================================================
UnitGroup* UnitGroup::TryGet(Cstring id)
{
	for(UnitGroup* group : groups)
	{
		if(group->id == id.s)
			return group;
	}
	return nullptr;
}

//=================================================================================================
UnitGroupList* UnitGroupList::TryGet(Cstring id)
{
	for(UnitGroupList* list : lists)
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
