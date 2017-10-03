#pragma once

//-----------------------------------------------------------------------------
struct Building;

//-----------------------------------------------------------------------------
struct BuildingGroup
{
	string id;
	vector<Building*> buildings;

	// Hardcoded building groups
	static BuildingGroup* BG_INN;
	static BuildingGroup* BG_HALL;
	static BuildingGroup* BG_TRAINING_GROUNDS;
	static BuildingGroup* BG_ARENA;
	static BuildingGroup* BG_FOOD_SELLER;
	static BuildingGroup* BG_ALCHEMIST;
	static BuildingGroup* BG_BLACKSMITH;
	static BuildingGroup* BG_MERCHANT;

	static vector<BuildingGroup*> groups;
	static BuildingGroup* TryGet(const AnyString& id);
	static BuildingGroup* Get(const AnyString& id)
	{
		auto group = TryGet(id);
		assert(group);
		return group;
	}
};
