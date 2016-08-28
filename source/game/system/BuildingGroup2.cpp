#include "Pch.h"
#include "Base.h"
#include "BuildingGroup2.h"
#include "GameSystem.h"
#include "GameSystemType.h"
#include "GameSystemManager.h"

vector<BuildingGroup2*> gs::building_groups;

class BuildingGroup2Handler : public GameSystemType
{
public:
	BuildingGroup2Handler() : GameSystemType(GameSystemTypeId::BuildingGroup, "building_group")
	{

	}
};

void BuildingGroup2::Register(GameSystemManager* gs_mgr)
{
	gs_mgr->AddType(new BuildingGroup2Handler);
}
