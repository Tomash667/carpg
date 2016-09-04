#pragma once

//-----------------------------------------------------------------------------
struct Building;
struct BuildingGroup;
struct BuildingScript;
struct UnitData;
enum class OLD_BUILDING;

//-----------------------------------------------------------------------------
namespace content
{
	extern vector<Building*> buildings;
	extern vector<BuildingGroup*> building_groups;
	extern vector<BuildingScript*> building_scripts;

	// Hardcoded building groups
	extern BuildingGroup* BG_INN;
	extern BuildingGroup* BG_HALL;
	extern BuildingGroup* BG_TRAINING_GROUNDS;
	extern BuildingGroup* BG_ARENA;
	extern BuildingGroup* BG_FOOD_SELLER;
	extern BuildingGroup* BG_ALCHEMIST;
	extern BuildingGroup* BG_BLACKSMITH;
	extern BuildingGroup* BG_MERCHANT;

	Building* FindBuilding(AnyString id);
	BuildingGroup* FindBuildingGroup(AnyString id);
	BuildingScript* FindBuildingScript(AnyString id);
	Building* FindOldBuilding(OLD_BUILDING type);
	UnitData* FindUnit(AnyString id);
}
