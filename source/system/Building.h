#pragma once

//-----------------------------------------------------------------------------
struct Building
{
	enum TileScheme
	{
		SCHEME_GRASS,
		SCHEME_BUILDING,
		SCHEME_SAND,
		SCHEME_PATH,
		SCHEME_UNIT,
		SCHEME_BUILDING_PART
	};

	enum Flags
	{
		FAVOR_CENTER = 1 << 0,
		FAVOR_ROAD = 1 << 1,
		HAVE_NAME = 1 << 2,
		LIST = 1 << 3
	};

	string id, name;
	Int2 size, shift[4];
	vector<TileScheme> scheme;
	int flags;
	BuildingGroup* group;
	Mesh* mesh, *inside_mesh;
	UnitData* unit;
	ResourceState state;

	Building() : size(0, 0), shift(), flags(0), mesh(nullptr), inside_mesh(nullptr), group(nullptr), unit(nullptr), state(ResourceState::NotLoaded) {}

	static vector<Building*> buildings;
	static Building* TryGet(Cstring id);
	static Building* Get(Cstring id)
	{
		auto building = TryGet(id);
		assert(building);
		return building;
	}
};

//-----------------------------------------------------------------------------
// Building used when constructing map
struct ToBuild
{
	Building* building;
	Int2 pt, unit_pt;
	GameDirection rot;
	bool required;

	ToBuild(Building* building, bool required = true) : building(building), required(required) {}
};
