#pragma once

struct Building;
struct BuildingGroup;
struct BuildingScript;
struct UnitData;
enum class OLD_BUILDING;

namespace content
{
	extern vector<Building*> buildings;
	extern vector<BuildingGroup> building_groups;
	extern vector<BuildingScript*> building_scripts;

	Building* FindBuilding(AnyString id);
	BuildingGroup* FindBuildingGroup(AnyString id);
	BuildingScript* FindBuildingScript(AnyString id);
	Building* FindOldBuilding(OLD_BUILDING type);
	UnitData* FindUnit(AnyString id);
}
