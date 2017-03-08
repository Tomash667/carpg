#pragma once

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

enum TILE_MODE : byte
{
	TM_NORMAL,
	TM_FIELD,
	TM_ROAD,
	TM_PATH,
	TM_BUILDING_SAND,
	TM_BUILDING, // wyœwiela siê jako budynek, nie ma fizyki ani kolizji
	TM_BUILDING_BLOCK, // wyœwietla siê jako budynek, ma fizykê i kolizje
	TM_MAX
};

struct TerrainTileInfo
{
	DWORD mask, shift;
	cstring name;
};

extern const TerrainTileInfo terrain_tile_info[];
extern cstring tile_mode_name[];

struct TerrainTile
{
	TERRAIN_TILE t, t2; // jeœli jest trawa to musi byæ pod t
	TILE_MODE mode;
	byte alpha; // w ilu % jest widoczny t2

	inline bool IsBlocking() const
	{
		return mode >= TM_BUILDING_BLOCK;
	}

	inline void Set(TERRAIN_TILE _t, TILE_MODE _mode)
	{
		t = _t;
		mode = _mode;
	}

	inline void Set(TERRAIN_TILE _t, TERRAIN_TILE _t2, byte _alpha, TILE_MODE _mode)
	{
		t = _t;
		t2 = _t2;
		alpha = _alpha;
		mode = _mode;
	}

	inline bool IsRoadOrPath() const
	{
		return mode == TM_ROAD || mode == TM_PATH;
	}

	inline bool IsBuilding() const
	{
		return mode >= TM_BUILDING;
	}

	inline cstring GetInfo() const
	{
		return Format("T:%s T2:%s Alpha:%d Mode:%s", terrain_tile_info[t].name, terrain_tile_info[t2].name, alpha, tile_mode_name[mode]);
	}
};
