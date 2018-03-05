#include "Pch.h"
#include "GameCore.h"
#include "UnitGroup.h"

//-----------------------------------------------------------------------------
vector<UnitGroup*> UnitGroup::groups;
vector<UnitGroupList*> UnitGroupList::lists;

//=================================================================================================
UnitGroup* UnitGroup::TryGet(const AnyString& id)
{
	for(auto group : groups)
	{
		if(group->id == id.s)
			return group;
	}

	return nullptr;
}

//=================================================================================================
UnitGroupList* UnitGroupList::TryGet(const AnyString& id)
{
	for(auto list : lists)
	{
		if(list->id == id.s)
			return list;
	}

	return nullptr;
}
