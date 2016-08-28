#include "Pch.h"
#include "Base.h"
#include "GameSystem.h"
#include "GameSystemManager.h"

#include "BuildingGroup2.h"

void RegisterAllTypes()
{
	GameSystemManager* gs_mgr = new GameSystemManager;

	BuildingGroup2::Register(gs_mgr);
}
