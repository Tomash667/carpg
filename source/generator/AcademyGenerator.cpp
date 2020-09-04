#include "Pch.h"
#include "AcademyGenerator.h"

#include "AIController.h"
#include "Building.h"
#include "GameCommon.h"
#include "Level.h"
#include "Object.h"
#include "OutsideLocation.h"
#include "Unit.h"

#include <Perlin.h>
#include <Terrain.h>

static const Int2 pt(63, 64);

//=================================================================================================
void AcademyGenerator::Init()
{
	OutsideLocationGenerator::Init();
	building = Building::Get("barracks");
}

//=================================================================================================
void AcademyGenerator::Generate()
{
	CreateMap();
	RandomizeTerrainTexture();

	// randomize height
	terrain->SetHeightMap(outside->h);
	terrain->RandomizeHeight(0.f, 3.f);
	float* h = terrain->GetHeightMap();

	for(uint y = 0; y < s; ++y)
	{
		for(uint x = 0; x < s; ++x)
		{
			if(x < 15 || x > s - 15 || y < 15 || y > s - 15)
				h[x + y * (s + 1)] += Random(10.f, 15.f);
		}
	}

	// create road
	for(uint y = 0; y < s / 2; ++y)
	{
		for(uint x = 62; x < 66; ++x)
		{
			outside->tiles[x + y * s].Set(TT_SAND, TM_ROAD);
			h[x + y * (s + 1)] = 1.f;
		}
	}
	for(uint y = 0; y < s / 2; ++y)
	{
		h[61 + y * (s + 1)] = 1.f;
		h[66 + y * (s + 1)] = 1.f;
		h[67 + y * (s + 1)] = 1.f;
	}

	terrain->RoundHeight();
	terrain->RoundHeight();

	Perlin perlin(4, 4);
	for(uint y = 0; y < s; ++y)
	{
		for(uint x = 0; x < s; ++x)
			h[x + y * (s + 1)] += (perlin.Get(1.f / 256 * x, 1.f / 256 * y) + 1.f) * 4;
	}

	// flatten road
	for(uint y = 1; y < s / 2; ++y)
	{
		for(uint x = 61; x <= 67; ++x)
			h[x + y * (s + 1)] = (h[x + y * (s + 1)] + h[x + 1 + y * (s + 1)] + h[x - 1 + y * (s + 1)] + h[x + (y - 1)*(s + 1)] + h[x + (y + 1)*(s + 1)]) / 5;
	}

	// set building tiles
	Int2 ext2 = building->size;
	vector<Int2> tmp_pts;

	const int x1 = (ext2.x - 1) / 2;
	const int x2 = ext2.x - x1 - 1;
	const int y1 = (ext2.y - 1) / 2;
	const int y2 = ext2.y - y1 - 1;

	int count = 0;
	float sum = 0;

	for(int yy = -y1, yr = 0; yy <= y2; ++yy, ++yr)
	{
		for(int xx = -x1, xr = 0; xx <= x2; ++xx, ++xr)
		{
			Building::TileScheme scheme = building->scheme[xr + (ext2.y - yr - 1)*ext2.x];

			Int2 pt2(pt.x + xx, pt.y + yy);

			TerrainTile& t = outside->tiles[pt2.x + (pt2.y)*s];

			switch(scheme)
			{
			case Building::SCHEME_GRASS:
			case Building::SCHEME_PATH:
				t.Set(TT_SAND, TM_NORMAL);
				break;
			case Building::SCHEME_BUILDING:
				t.Set(TT_SAND, TM_BUILDING_BLOCK);
				break;
			case Building::SCHEME_UNIT:
				unit_pos = Vec3(2.f*pt2.x, 0, 2.f*pt2.y);
				t.Set(TT_SAND, TM_BUILDING_SAND);
				break;
			case Building::SCHEME_SAND:
				t.Set(TT_SAND, TM_BUILDING_SAND);
				break;
			case Building::SCHEME_BUILDING_PART:
				t.Set(TT_SAND, TM_BUILDING);
				break;
			default:
				assert(0);
				break;
			}
		}
	}

	// normalize height around building
	for(int yy = -y1 - 1, yr = 0; yy <= y2 + 2; ++yy, ++yr)
	{
		for(int xx = -x1 - 1, xr = 0; xx <= x2 + 2; ++xx, ++xr)
		{
			Int2 pt2(pt.x + xx, pt.y + yy);
			++count;
			sum += outside->h[pt2.x + pt2.y*(s + 1)];
			tmp_pts.push_back(pt2);
		}
	}
	sum /= count;
	for(Int2& pt : tmp_pts)
		outside->h[pt.x + pt.y*(s + 1)] = sum;
	tmp_pts.clear();

	// it would be better to round only next to building...
	terrain->RoundHeight();

	// normalize height around building (after rounding)
	sum = 0;
	count = 0;
	for(int yy = -y1, yr = 0; yy <= y2 + 1; ++yy, ++yr)
	{
		for(int xx = -x1, xr = 0; xx <= x2 + 1; ++xx, ++xr)
		{
			Int2 pt2(pt.x + xx, pt.y + yy);
			++count;
			sum += outside->h[pt2.x + pt2.y*(s + 1)];
			tmp_pts.push_back(pt2);
		}
	}
	sum /= count;
	for(Int2& pt : tmp_pts)
		outside->h[pt.x + pt.y*(s + 1)] = sum;
	tmp_pts.clear();

	terrain->RemoveHeightMap();
}

//=================================================================================================
void AcademyGenerator::GenerateObjects()
{
	SpawnBuilding(true);
	SpawnForestObjects(1);
}

//=================================================================================================
void AcademyGenerator::SpawnBuilding(bool first)
{
	Vec3 pos = Vec3(float(pt.x + building->shift[GDIR_DOWN].x) * 2, 1.f, float(pt.y + building->shift[GDIR_DOWN].y) * 2);
	terrain->SetY(pos);

	if(first)
	{
		Object* o = new Object;
		o->rot = Vec3(0, 0, 0);
		o->pos = pos;
		o->scale = 1.f;
		o->base = nullptr;
		o->mesh = building->mesh;
		outside->objects.push_back(o);
	}

	game_level->ProcessBuildingObjects(*outside, nullptr, nullptr, building->mesh, nullptr, 0.f, GDIR_DOWN,
		pos, building, nullptr, !first);
}

//=================================================================================================
void AcademyGenerator::GenerateItems()
{
	SpawnForestItems(-1);
}

//=================================================================================================
void AcademyGenerator::GenerateUnits()
{
	UnitData* ud = UnitData::Get("q_main_academy");
	game_level->SpawnUnitNearLocation(*outside, unit_pos, *ud);
}

//=================================================================================================
void AcademyGenerator::OnEnter()
{
	OutsideLocationGenerator::OnEnter();

	if(!first)
		SpawnBuilding(false);

	SpawnCityPhysics();
}

//=================================================================================================
void AcademyGenerator::SpawnTeam()
{
	game_level->AddPlayerTeam(Vec3(128.f, 0, 80.f), PI);
}

//=================================================================================================
void AcademyGenerator::OnLoad()
{
	OutsideLocationGenerator::OnLoad();
	SpawnBuilding(false);
	SpawnCityPhysics();
}
