#pragma once

struct Building;
struct BuildingScript;
struct UnitData;
enum class OLD_BUILDING;

namespace content
{
	vector<Building*> buildings;
	vector<string> building_groups;
	vector<BuildingScript*> building_scripts;

	Building* FindBuilding(AnyString id);
	int FindBuildingGroup(AnyString id);
	BuildingScript* FindBuildingScript(AnyString id);
	Building* FindOldBuilding(OLD_BUILDING type);
	UnitData* FindUnit(AnyString id);
}
