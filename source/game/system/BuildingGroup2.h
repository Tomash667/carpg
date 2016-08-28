#pragma once

class GameSystemManager;
struct Building2;

struct BuildingGroup2
{
	string name;
	vector<Building2*> buildings;

	static void Register(GameSystemManager* gs_mgr);
};
