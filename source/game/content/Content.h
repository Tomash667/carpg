#pragma once

struct Building;

namespace content
{
	vector<Building*> buildings;
	vector<string> building_groups;

	Building* FindBuilding(AnyString id);
	int FindBuildingGroup(AnyString id);
}
