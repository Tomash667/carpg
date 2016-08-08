#include "Pch.h"
#include "Base.h"
#include "Game.h"
#include "DatatypeManager.h"
// all datatypes
#include "Building.h"
#include "BuildingGroup.h"
#include "BuildingScript.h"
#include "UnitData.h"

extern string g_system_dir;
extern string g_lang_prefix;

void Game::InitializeDatatypeManager()
{
	dt_mgr = new DatatypeManager(g_system_dir.c_str(), Format("%s/lang/%s", g_system_dir.c_str(), g_lang_prefix.c_str()));
	DatatypeManager& r_dt_mgr = *dt_mgr;

	Building::Register(r_dt_mgr);
	BuildingGroup::Register(r_dt_mgr);
	BuildingScript::Register(r_dt_mgr);
	UnitData::Register(r_dt_mgr);

	if(!dt_mgr->LoadFilelist())
		throw "Failed to load datatype filelist.";
}

void Game::CleanupDatatypeManager()
{
	delete dt_mgr;
}
