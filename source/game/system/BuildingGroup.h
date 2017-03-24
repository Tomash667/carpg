#pragma once

//-----------------------------------------------------------------------------
struct Building;
class GameTypeManager;

//-----------------------------------------------------------------------------
// Building group
struct BuildingGroup
{
	string id;
	vector<Building*> buildings;

	static void Register(GameTypeManager& gt_mgr);
};
