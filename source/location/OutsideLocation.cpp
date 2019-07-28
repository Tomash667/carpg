#include "Pch.h"
#include "GameCore.h"
#include "OutsideLocation.h"
#include "SaveState.h"
#include "Unit.h"
#include "Object.h"
#include "Chest.h"
#include "GroundItem.h"
#include "GameFile.h"
#include "BitStreamFunc.h"
#include "Level.h"

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
	DeleteElements(objects);
	DeleteElements(units);
	DeleteElements(chests);
	DeleteElements(usables);
	DeleteElements(items);
}

//=================================================================================================
void OutsideLocation::Apply(vector<std::reference_wrapper<LevelArea>>& areas)
{
	areas.push_back(*this);
}

//=================================================================================================
void OutsideLocation::Save(GameWriter& f, bool local)
{
	Location::Save(f, local);

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
void OutsideLocation::Load(GameReader& f, bool local, LOCATION_TOKEN token)
{
	Location::Load(f, local, token);

	if(last_visit != -1)
	{
		if(LOAD_VERSION >= V_0_11)
			LevelArea::Load(f, local);
		else
			LevelArea::Load(f, local, old::LoadCompatibility::OutsideLocation);

		// terrain
		int size2 = size + 1;
		size2 *= size2;
		h = new float[size2];
		tiles = new TerrainTile[size*size];
		f.Read(tiles, sizeof(TerrainTile)*size*size);
		f.Read(h, sizeof(float)*size2);
	}

	if(LOAD_VERSION < V_0_8 && st == 20 && type == L_FOREST)
		state = LS_HIDDEN;
}

//=================================================================================================
void OutsideLocation::Write(BitStreamWriter& f)
{
	f.Write((cstring)tiles, sizeof(TerrainTile)*size*size);
	f.Write((cstring)h, sizeof(float)*(size + 1)*(size + 1));
	f << L.light_angle;

	LevelArea::Write(f);

	WritePortals(f);
}

//=================================================================================================
bool OutsideLocation::Read(BitStreamReader& f)
{
	int size11 = size*size;
	int size22 = size + 1;
	size22 *= size22;
	if(!tiles)
		tiles = new TerrainTile[size11];
	if(!h)
		h = new float[size22];
	f.Read((char*)tiles, sizeof(TerrainTile)*size11);
	f.Read((char*)h, sizeof(float)*size22);
	f >> L.light_angle;
	if(!f)
	{
		Error("Read level: Broken packet for terrain.");
		return false;
	}

	if(!LevelArea::Read(f))
		return false;

	// portals
	if(!ReadPortals(f, L.dungeon_level))
	{
		Error("Read level: Broken portals.");
		return false;
	}

	return true;
}

//=================================================================================================
void OutsideLocation::BuildRefidTables()
{
	LevelArea::BuildRefidTables();
}

//=================================================================================================
bool OutsideLocation::FindUnit(Unit* unit, int* level)
{
	assert(unit);

	for(Unit* u : units)
	{
		if(unit == u)
		{
			if(level)
				*level = -1;
			return true;
		}
	}

	return false;
}

//=================================================================================================
Unit* OutsideLocation::FindUnit(UnitData* data, int& at_level)
{
	assert(data);

	for(Unit* u : units)
	{
		if(u->data == data)
		{
			at_level = -1;
			return u;
		}
	}

	return nullptr;
}
