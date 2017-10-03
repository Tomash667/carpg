#include "Pch.h"
#include "Core.h"
#include "BuildingGroup.h"

//-----------------------------------------------------------------------------
vector<BuildingGroup*> BuildingGroup::groups;
BuildingGroup* BuildingGroup::BG_INN;
BuildingGroup* BuildingGroup::BG_HALL;
BuildingGroup* BuildingGroup::BG_TRAINING_GROUNDS;
BuildingGroup* BuildingGroup::BG_ARENA;
BuildingGroup* BuildingGroup::BG_FOOD_SELLER;
BuildingGroup* BuildingGroup::BG_ALCHEMIST;
BuildingGroup* BuildingGroup::BG_BLACKSMITH;
BuildingGroup* BuildingGroup::BG_MERCHANT;

//=================================================================================================
BuildingGroup* BuildingGroup::TryGet(const AnyString& id)
{
	for(auto group : groups)
	{
		if(group->id == id)
			return group;
	}

	return nullptr;
}
