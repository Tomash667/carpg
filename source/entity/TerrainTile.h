#pragma once

//-----------------------------------------------------------------------------
// u¿ywane w FindPath
enum TERRAIN_TILE : byte
{
	TT_GRASS,
	TT_GRASS2,
	TT_GRASS3,
	TT_FIELD,
	TT_SAND,
	TT_ROAD,
	TT_MAX
};

//-----------------------------------------------------------------------------
enum TILE_MODE : byte
{
	TM_NORMAL,
	TM_FIELD,
	TM_ROAD,
	TM_PATH,
	TM_BUILDING_SAND,
	TM_BUILDING, // wyœwiela siê jako budynek, nie ma fizyki ani kolizji
	TM_BUILDING_BLOCK, // wyœwietla siê jako budynek, ma fizykê i kolizje
	TM_NO_GRASS,
	TM_MAX
};

//-----------------------------------------------------------------------------
struct TerrainTileInfo
{
	uint mask, shift;
	cstring name;
};

//-----------------------------------------------------------------------------
extern const TerrainTileInfo terrainTileInfo[];
extern cstring tileModeName[];

//-----------------------------------------------------------------------------
struct TerrainTile
{
	TERRAIN_TILE t, t2; // jeœli jest trawa to musi byæ pod t
	TILE_MODE mode;
	byte alpha; // w ilu % jest widoczny t2

	bool IsBlocking() const
	{
		return mode >= TM_BUILDING_BLOCK && mode != TM_NO_GRASS;
	}

	void Set(TERRAIN_TILE t, TILE_MODE mode)
	{
		this->t = t;
		this->mode = mode;
	}

	void Set(TERRAIN_TILE t, TERRAIN_TILE t2, byte alpha, TILE_MODE mode)
	{
		this->t = t;
		this->t2 = t2;
		this->alpha = alpha;
		this->mode = mode;
	}

	bool IsRoadOrPath() const
	{
		return mode == TM_ROAD || mode == TM_PATH;
	}

	bool IsBuilding() const
	{
		return mode >= TM_BUILDING && mode != TM_NO_GRASS;
	}

	cstring GetInfo() const
	{
		return Format("T:%s T2:%s Alpha:%d Mode:%s", terrainTileInfo[t].name, terrainTileInfo[t2].name, alpha, tileModeName[mode]);
	}
};
