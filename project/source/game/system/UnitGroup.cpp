#include "Pch.h"
#include "Core.h"
#include "UnitGroup.h"

//-----------------------------------------------------------------------------
vector<UnitGroup*> UnitGroup::groups;

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
