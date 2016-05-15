#pragma once

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
// building groups
extern int BG_INN;
extern int BG_HALL;
extern int BG_TRAINING_GROUND;
extern int BG_ARENA;
extern int BG_FOOD_SELLER;
extern int BG_ALCHEMIST;

//-----------------------------------------------------------------------------
// Bazowy budynek
struct Building
{
	enum Flags
	{
		FAVOR_CENTER = 1 << 0,
		FAVOR_ROAD = 1 << 1,
		HAVE_NAME = 1 << 2
	};

	string id, mesh_id, inside_mesh_id;
	cstring name;
	INT2 size, shift[4];
	vector<TileScheme> scheme;
	int flags, group;
	Animesh* mesh, *inside_mesh;

	Building() : name(nullptr), size(0, 0), shift(), flags(0), mesh(nullptr), inside_mesh(nullptr), group(-1) {}
};

//-----------------------------------------------------------------------------
// building used when constructing map
struct ToBuild
{
	Building* type;
	INT2 pt, unit_pt;
	int rot;
	bool required;

	ToBuild(Building* type, bool required = true) : type(type), required(required) {}
};

struct BuildPt
{
	INT2 pt;
	int side; // 0 = obojêtnie, 1 = <==> szerszy, 2 ^ d³u¿szy
};

struct Road
{
	INT2 start, end;
	int w, dir, length;
	bool side_used[4];
};

struct APoint2
{
	int koszt, stan, dir;
	INT2 prev;
};

struct APoint2Sorter
{
	APoint2Sorter(APoint2* _grid, uint _s) : grid(_grid), s(_s)
	{
	}

	bool operator() (int idx1, int idx2) const
	{
		return grid[idx1].koszt > grid[idx2].koszt;
	}

	APoint2* grid;
	uint s;
};

enum BsCode
{
	BS_ADD,
	BS_RANDOM,
	BS_NORMAL,
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
	BS_ELSE,
	BS_ENDIF,
	BS_RANDOM,
	BS_CHECK,
	BS_OP_ADD,
	BS_OP_SUB,
	BS_OP_MUL,
	BS_OP_DIV,
	BS_OP_GREATER,
	BS_OP_GREATER_EQ,
	BS_OP_LESS,
	BS_OP_LESS_EQ,
	BS_OP_EQ,
	BS_OP_NOT_EQ
};

struct BuildingScript
{
	struct Variant
	{
		int vars;
		vector<BsCode> code;
	};
};
