#pragma once

//-----------------------------------------------------------------------------
struct Building;

//-----------------------------------------------------------------------------
// Building group
struct BuildingGroup
{
	string id;
	vector<Building*> buildings;
};
