#pragma once

//-----------------------------------------------------------------------------
struct Building;
class DatatypeManager;

//-----------------------------------------------------------------------------
// Building group
struct BuildingGroup
{
	string id;
	vector<Building*> buildings;

	static void Register(DatatypeManager& dt_mgr);
};
