#pragma once

//-----------------------------------------------------------------------------
#define SCHEME_GRASS 0
#define SCHEME_BUILDING 1
#define SCHEME_SAND 2
#define SCHEME_PATH 3
#define SCHEME_UNIT 4
#define SCHEME_BUILDING_PART 5
#define SCHEME_BUILDING_NO_PHY 6

//-----------------------------------------------------------------------------
// Typy budynków
enum BUILDING
{
	B_MERCHANT,
	// kupiec, siedzi na sto³ku lub stoi przy ladzie
	// dostêpny w ka¿dym mieœcie i wiosce, sprzedaje s³absze przedmioty, mikstury i inne przedmioty
	B_BLACKSMITH,
	// kowal, robi coœ na kowadle albo stoi przy ladzie
	// 50% dostêpnoœci, sprzedaje broñ i przedmioty w zale¿noœci od rozmiaru miasta
	B_ALCHEMIST,
	// alchemik, robi coœ przy miksturach albo stoi przy ladzie
	// 50% dostêpnoœci, sprzedaje mikstury
	B_TRAINING_GROUND,
	// pola treningowe, instruktor siedzi na ³awce albo trenuje z innymi
	// 50% szans, mo¿na tu trenowaæ
	B_INN,
	// karczma, barman siedzi na sto³ku albo stoi przy ladzie
	// zawsze dostêpny, mo¿na tu odpoczywaæ/kurowaæ siê
	B_CITY_HALL,
	// ratusz, jest tu burmistrz, przywódca stra¿y oraz 2 stra¿ników, stra¿nicy stoj¹ przy wejœciu (mapa w zeszycie)
	// burmistrz zleca zadania i siedzie w tym samym miejscu
	// przywódca stra¿y ³azi pomiêdzy barakami, kowalem a tym miejscem i odwala papierkow¹ robotê
	// mo¿na tu dostaæ questy, przywódca stra¿y te¿ mo¿e siê do czegoœ przyda
	B_VILLAGE_HALL,
	// dom so³tysa, jest tu so³tys (mapa w zeszycie), siedzi ci¹gle w tym samym miejscu
	// daje questy
	B_BARRACKS,
	// tu trenuj¹ stra¿nicy, na pacho³kach, na tarczach strzelniczych i pomiêdzy sob¹
	B_HOUSE,
	B_HOUSE2,
	B_HOUSE3,
	// st¹d wy³a¿¹ mieszkañcy, nie da siê wejœæ
	B_ARENA,
	// arena, dostêpna tylko w mieœcie
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
// grupy budynków
enum BUILDING_GROUP
{
	BG_INN,
	BG_HALL,
	BG_NONE
};

//-----------------------------------------------------------------------------
// Building flags
enum BUILDING_FLAGS
{
	BF_FAVOR_CENTER = 1<<1,
	BF_FAVOR_ROAD = 1<<2,
	BF_DRAW_NAME = 1<<3,
	BF_LOAD_NAME = 1<<4,
};

//-----------------------------------------------------------------------------
// Bazowy budynek
struct Building
{
	cstring id, name, mesh_id, inside_mesh_id;
	INT2 size, shift[4];
	int* scheme, flags;
	Animesh* mesh, *inside_mesh;
	BUILDING_GROUP group;

	Building(cstring id, cstring mesh_id, cstring inside_mesh_id, const INT2& size, const INT2& shift0, const INT2& shift1, const INT2& shift2,
		const INT2& shift3, int* scheme, int flags, BUILDING_GROUP group=BG_NONE) :
		id(id), name(nullptr), mesh_id(mesh_id), inside_mesh_id(inside_mesh_id), size(size), scheme(scheme), flags(flags), mesh(nullptr),
		inside_mesh(nullptr), group(group)
	{
		shift[0] = shift0;
		shift[1] = shift1;
		shift[2] = shift2;
		shift[3] = shift3;
	}
};
extern Building buildings[];

//-----------------------------------------------------------------------------
// building used when constructing map
struct ToBuild
{
	BUILDING type;
	INT2 ext;
	int* scheme;
	INT2 pt, unit_pt;
	int rot;
	bool favor_center, favor_road, required;

	ToBuild(BUILDING _type, bool required=true) : type(_type), required(required)
	{
		const Building& b = buildings[type];
		ext = b.size;
		favor_center = IS_SET(b.flags, BF_FAVOR_CENTER);
		favor_road = IS_SET(b.flags, BF_FAVOR_ROAD);
		scheme = b.scheme;
	}
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
