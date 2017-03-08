#include "Pch.h"
#include "Base.h"
#include "Game.h"
#include "GameTypeManager.h"
// all gametypes
#include "Building.h"
#include "BuildingGroup.h"
#include "BuildingScript.h"
#include "UnitData.h"

extern string g_system_dir;
extern string g_lang_prefix;

void Game::InitializeGameTypeManager()
{
	gt_mgr = new GameTypeManager(g_system_dir.c_str(), Format("%s/lang/%s", g_system_dir.c_str(), g_lang_prefix.c_str()));
	GameTypeManager& r_dt_mgr = *gt_mgr;

	Building::Register(r_dt_mgr);
	BuildingGroup::Register(r_dt_mgr);
	BuildingScript::Register(r_dt_mgr);
	UnitData::Register(r_dt_mgr);

	if(!gt_mgr->LoadFilelist())
		throw "Failed to load gametype filelist.";
}

void Game::CleanupGameTypeManager()
{
	delete gt_mgr;
}
