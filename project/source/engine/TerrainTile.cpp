#include "Pch.h"
#include "Base.h"
#include "TerrainTile.h"

extern const TerrainTileInfo terrain_tile_info[TT_MAX] = {
	0x00000000, 0,	"TT_GRASS",
	0x0000FF00,	8,	"TT_GRASS2",
	0x000000FF,	0,	"TT_GRASS3",
	0x000000FF,	0,	"TT_FIELD",
	0x00FF0000,	16,	"TT_SAND",
	0xFF000000,	24,	"TT_ROAD",
};

extern cstring tile_mode_name[TM_MAX] = {
	"TM_NORMAL",
	"TM_FIELD",
	"TM_ROAD",
	"TM_PATH",
	"TM_BUILDING_SAND",
	"TM_BUILDING",
	"TM_BUILDING_BLOCK"
};
