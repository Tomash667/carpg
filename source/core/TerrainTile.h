#pragma once

// u�ywane w FindPath
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
	TM_BUILDING, // wy�wiela si� jako budynek, nie ma fizyki ani kolizji
	TM_BUILDING_BLOCK, // wy�wietla si� jako budynek, ma fizyk� i kolizje
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
	TERRAIN_TILE t, t2; // je�li jest trawa to musi by� pod t
	TILE_MODE mode;
	byte alpha; // w ilu % jest widoczny t2

	bool IsBlocking() const
	{
		return mode >= TM_BUILDING_BLOCK;
	}

	void Set(TERRAIN_TILE _t, TILE_MODE _mode)
	{
		t = _t;
		mode = _mode;
	}

	void Set(TERRAIN_TILE _t, TERRAIN_TILE _t2, byte _alpha, TILE_MODE _mode)
	{
		t = _t;
		t2 = _t2;
		alpha = _alpha;
		mode = _mode;
	}

	bool IsRoadOrPath() const
	{
		return mode == TM_ROAD || mode == TM_PATH;
	}

	bool IsBuilding() const
	{
		return mode >= TM_BUILDING;
	}

	cstring GetInfo() const
	{
		return Format("T:%s T2:%s Alpha:%d Mode:%s", terrain_tile_info[t].name, terrain_tile_info[t2].name, alpha, tile_mode_name[mode]);
	}
};
