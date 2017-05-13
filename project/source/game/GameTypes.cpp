#include "Pch.h"
#include "Base.h"
#include "Game.h"
#include "TypeManager.h"

extern string g_system_dir;
extern string g_lang_prefix;

Type* CreateBuildingHandler();
Type* CreateBuildingGroupHandler();
Type* CreateBuildingScriptHandler();
Type* CreateUnitHandler();

void Game::InitializeTypeManager()
{
	type_manager = new TypeManager(g_system_dir.c_str(), Format("%s/lang/%s", g_system_dir.c_str(), g_lang_prefix.c_str()));

	type_manager->Add(CreateBuildingHandler());
	type_manager->Add(CreateBuildingGroupHandler());
	type_manager->Add(CreateBuildingScriptHandler());
	type_manager->Add(CreateUnitHandler());

	type_manager->OrderDependencies();
	
	if(!type_manager->LoadFilelist())
		throw "Failed to load type filelist.";
}

void Game::CleanupTypeManager()
{
	delete type_manager;
}
