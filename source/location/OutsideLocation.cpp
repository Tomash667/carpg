#include "Pch.h"
#include "OutsideLocation.h"

#include "BitStreamFunc.h"
#include "Chest.h"
#include "GroundItem.h"
#include "Level.h"
#include "Object.h"
#include "Unit.h"

namespace OLD
{
	enum TERRAIN_TILE : byte
	{
		TT_GRASS,
		TT_GRASS2,
		TT_GRASS3,
		TT_SAND,
		TT_ROAD,
		TT_BUILD_GRASS,
		TT_BUILD_SAND,
		TT_BUILD_ROAD
	};

	const int TM_BUILDING_NO_PHY = 7;
}

//=================================================================================================
OutsideLocation::OutsideLocation() : Location(true), LevelArea(LevelArea::Type::Outside, LevelArea::OUTSIDE_ID, true), tiles(nullptr), h(nullptr)
{
	mine = Int2(0, 0);
	maxe = Int2(size, size);
}

//=================================================================================================
OutsideLocation::~OutsideLocation()
{
	delete[] tiles;
	delete[] h;
}

//=================================================================================================
void OutsideLocation::Apply(vector<std::reference_wrapper<LevelArea>>& areas)
{
	areas.push_back(*this);
}

//=================================================================================================
void OutsideLocation::Save(GameWriter& f)
{
	Location::Save(f);

	if(last_visit != -1)
	{
		LevelArea::Save(f);

		// terrain
		f.Write(tiles, sizeof(TerrainTile)*size*size);
		int size2 = size + 1;
		size2 *= size2;
		f.Write(h, sizeof(float)*size2);
	}
}

//=================================================================================================
void OutsideLocation::Load(GameReader& f)
{
	Location::Load(f);

	if(last_visit != -1)
	{
		if(LOAD_VERSION >= V_0_11)
			LevelArea::Load(f);
		else
			LevelArea::Load(f, old::LoadCompatibility::OutsideLocation);

		// terrain
		int size2 = size + 1;
		size2 *= size2;
		h = new float[size2];
		tiles = new TerrainTile[size*size];
		f.Read(tiles, sizeof(TerrainTile)*size*size);
		f.Read(h, sizeof(float)*size2);
	}

	if(LOAD_VERSION < V_0_8 && st == 20 && type == L_OUTSIDE)
		state = LS_HIDDEN;
}

//=================================================================================================
void OutsideLocation::Write(BitStreamWriter& f)
{
	f.Write((cstring)tiles, sizeof(TerrainTile)*size*size);
	f.Write((cstring)h, sizeof(float)*(size + 1)*(size + 1));
	f << game_level->light_angle;

	LevelArea::Write(f);

	WritePortals(f);
}

//=================================================================================================
bool OutsideLocation::Read(BitStreamReader& f)
{
	int size11 = size * size;
	int size22 = size + 1;
	size22 *= size22;
	if(!tiles)
		tiles = new TerrainTile[size11];
	if(!h)
		h = new float[size22];
	f.Read((char*)tiles, sizeof(TerrainTile)*size11);
	f.Read((char*)h, sizeof(float)*size22);
	f >> game_level->light_angle;
	if(!f)
	{
		Error("Read level: Broken packet for terrain.");
		return false;
	}

	if(!LevelArea::Read(f))
		return false;

	// portals
	if(!ReadPortals(f, game_level->dungeon_level))
	{
		Error("Read level: Broken portals.");
		return false;
	}

	return true;
}
