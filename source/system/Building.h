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
		LIST = 1 << 3,
		FAVOR_DIST = 1 << 4,
		NO_PATH = 1 << 5
	};

	string id, name;
	Int2 size, shift[4];
	vector<TileScheme> scheme;
	int flags;
	BuildingGroup* group;
	Mesh* mesh, *insideMesh;
	UnitData* unit;
	ResourceState state;

	Building() : size(0, 0), shift{Int2::Zero, Int2::Zero, Int2::Zero, Int2::Zero}, flags(0), mesh(nullptr), insideMesh(nullptr), group(nullptr),
		unit(nullptr), state(ResourceState::NotLoaded) {}

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
	Int2 pt, unitPt;
	GameDirection dir;
	bool required;

	ToBuild(Building* building, bool required = true) : building(building), required(required) {}
};
