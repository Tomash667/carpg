#include "Pch.h"
#include "Base.h"
#include "BuildingGroup.h"
#include "Content.h"
#include "TypeVectorContainer.h"

//-----------------------------------------------------------------------------
vector<BuildingGroup*> content::building_groups;

//-----------------------------------------------------------------------------
// Hardcoded building groups
BuildingGroup* content::BG_INN;
BuildingGroup* content::BG_HALL;
BuildingGroup* content::BG_TRAINING_GROUNDS;
BuildingGroup* content::BG_ARENA;
BuildingGroup* content::BG_FOOD_SELLER;
BuildingGroup* content::BG_ALCHEMIST;
BuildingGroup* content::BG_BLACKSMITH;
BuildingGroup* content::BG_MERCHANT;

//=================================================================================================
// Find building group by id
//=================================================================================================
BuildingGroup* content::FindBuildingGroup(const AnyString& id)
{
	for(BuildingGroup* group : building_groups)
	{
		if(group->id == id.s)
			return group;
	}
	return nullptr;
}

class BuildingGroupHandler : public TypeImpl<BuildingGroup>
{
public:
	BuildingGroupHandler() : TypeImpl(TypeId::BuildingGroup, "building_group", "Building group", "buildings")
	{
		AddId(offsetof(BuildingGroup, id));

		container = new TypeVectorContainer(this, content::building_groups);
	}

	void AfterLoad() override
	{
		content::BG_INN = content::FindBuildingGroup("inn");
		content::BG_HALL = content::FindBuildingGroup("hall");
		content::BG_TRAINING_GROUNDS = content::FindBuildingGroup("training_grounds");
		content::BG_ARENA = content::FindBuildingGroup("arena");
		content::BG_FOOD_SELLER = content::FindBuildingGroup("food_seller");
		content::BG_ALCHEMIST = content::FindBuildingGroup("alchemist");
		content::BG_BLACKSMITH = content::FindBuildingGroup("blacksmith");
		content::BG_MERCHANT = content::FindBuildingGroup("merchant");
	}
};

Type* CreateBuildingGroupHandler()
{
	return new BuildingGroupHandler;
}
