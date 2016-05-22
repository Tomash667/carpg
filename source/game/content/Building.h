#pragma once

//-----------------------------------------------------------------------------
struct UnitData;

//-----------------------------------------------------------------------------
// Old building enum - required for pre 0.5 compability
enum class OLD_BUILDING
{
	B_MERCHANT,
	B_BLACKSMITH,
	B_ALCHEMIST,
	B_TRAINING_GROUND,
	B_INN,
	B_CITY_HALL,
	B_VILLAGE_HALL,
	B_BARRACKS,
	B_HOUSE,
	B_HOUSE2,
	B_HOUSE3,
	B_ARENA,
	B_FOOD_SELLER,
	B_COTTAGE,
	B_COTTAGE2,
	B_COTTAGE3,
	B_VILLAGE_INN,
	B_NONE,
	B_VILLAGE_HALL_OLD,
	B_MAX
};

//-----------------------------------------------------------------------------
// Building group
struct BuildingGroup
{
	string id;
	uint count;
};

//-----------------------------------------------------------------------------
// Base building
struct Building
{
	enum TileScheme
	{
		SCHEME_GRASS,
		SCHEME_BUILDING,
		SCHEME_SAND,
		SCHEME_PATH,
		SCHEME_UNIT,
		SCHEME_BUILDING_PART,
		SCHEME_BUILDING_NO_PHY
	};

	enum Flags
	{
		FAVOR_CENTER = 1 << 0,
		FAVOR_ROAD = 1 << 1,
		HAVE_NAME = 1 << 2,
		LIST = 1 << 3
	};

	string id, mesh_id, inside_mesh_id, name;
	INT2 size, shift[4];
	vector<TileScheme> scheme;
	int flags;
	BuildingGroup* group;
	Animesh* mesh, *inside_mesh;
	UnitData* unit;
	bool name_set;

	Building() : size(0, 0), shift(), flags(0), mesh(nullptr), inside_mesh(nullptr), group(nullptr), name_set(false), unit(nullptr) {}
};

//-----------------------------------------------------------------------------
// Building used when constructing map
struct ToBuild
{
	Building* type;
	INT2 pt, unit_pt;
	int rot;
	bool required;

	ToBuild(Building* type, bool required = true) : type(type), required(required) {}
};

//-----------------------------------------------------------------------------
// Building script
struct BuildingScript
{
	enum Code
	{
		BS_ADD,
		BS_RANDOM,
		BS_BUILDING,
		BS_GROUP,
		BS_INT,
		BS_VAR,
		BS_SHUFFLE_START,
		BS_SHUFFLE_END,
		BS_REQUIRED_ON,
		BS_REQUIRED_OFF,
		BS_SET_VAR,
		BS_SET_VAR_OP,
		BS_INC,
		BS_DEC,
		BS_IF,
		BS_IF_RANDOM,
		BS_ELSE,
		BS_ENDIF,
		BS_EQUAL,
		BS_NOT_EQUAL,
		BS_GREATER,
		BS_GREATER_EQUAL,
		BS_LESS,
		BS_LESS_EQUAL
	};

	struct Variant
	{
		uint index, vars;
		vector<int> code;
	};

	static const uint MAX_VARS = 10;

	string id;
	vector<Variant*> variants;
	Variant* variant;

	~BuildingScript()
	{
		DeleteElements(variants);
	}

	bool HaveBuilding(const string& group_id) const;
	bool HaveBuilding(BuildingGroup* group, Variant* variant) const;

private:
	bool IsEntryGroup(const int*& code, BuildingGroup* group) const;
};

//-----------------------------------------------------------------------------
// Hardcoded building groups
extern BuildingGroup* BG_INN;
extern BuildingGroup* BG_TRAINING_GROUND;
extern BuildingGroup* BG_ARENA;
extern BuildingGroup* BG_FOOD_SELLER;
extern BuildingGroup* BG_ALCHEMIST;
