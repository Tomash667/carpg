#pragma once

//-----------------------------------------------------------------------------
enum TILE_TYPE : byte
{
	UNUSED,
	EMPTY,
	STAIRS_UP,
	STAIRS_DOWN,
	DOORS,
	HOLE_FOR_DOORS,
	BARS_FLOOR,
	BARS_CEILING,
	BARS,
	WALL,
	BLOCKADE,
	BLOCKADE_WALL,
	USED
};

//-----------------------------------------------------------------------------
struct Tile
{
	enum FLAGS
	{
		F_FLOOR = 0x1,
		F_CEILING = 0x2,
		F_LOW_CEILING = 0x4,
		F_BARS_FLOOR = 0x8,
		F_BARS_CEILING = 0x10,

		// unused 0x20 0x40 0x80

		F_WALL_LEFT = 0x100,
		F_WALL_RIGHT = 0x200,
		F_WALL_FRONT = 0x400,
		F_WALL_BACK = 0x800,

		F_CEIL_LEFT = 0x1000,
		F_CEIL_RIGHT = 0x2000,
		F_CEIL_FRONT = 0x4000,
		F_CEIL_BACK = 0x8000,

		F_UP_LEFT = 0x10000,
		F_UP_RIGHT = 0x20000,
		F_UP_FRONT = 0x40000,
		F_UP_BACK = 0x80000,

		F_HOLE_LEFT = 0x100000,
		F_HOLE_RIGHT = 0x200000,
		F_HOLE_FRONT = 0x400000,
		F_HOLE_BACK = 0x800000, // 1<<21

		F_SPECIAL = 1 << 29, // used to mark prison doors
		F_SECOND_TEXTURE = 1 << 30,
		F_REVEALED = 1 << 31
	};

	int flags;
	word room;
	TILE_TYPE type;
	// jeszcze jest 1-2 bajty miejsca na coœ :o (jak pokój bêdzie byte)

	// DDDDGGGGRRRRSSSS000KKNSP
	// ####    ####    ########
	//     ####    ####
	// œciany >> 8
	// podsufit >> 12
	// góra >> 16
	// dó³ >> 20

	bool IsWall() const
	{
		return type == WALL || type == BLOCKADE_WALL;
	}

	static void SetupFlags(Tile* tiles, const Int2& size);
	static void DebugDraw(Tile* tiles, const Int2& size);
};

//-----------------------------------------------------------------------------
// Is blocking movement
inline bool IsBlocking(TILE_TYPE p)
{
	return p >= WALL || p == UNUSED;
}
inline bool IsBlocking(const Tile& p)
{
	return IsBlocking(p.type);
}
// Is block movement or objects should not be placed here to block path
inline bool IsBlocking2(TILE_TYPE p)
{
	return !(p == EMPTY || p == BARS || p == BARS_FLOOR || p == BARS_CEILING);
}
inline bool IsBlocking2(const Tile& p)
{
	return IsBlocking2(p.type);
}
inline bool IsEmpty(TILE_TYPE p)
{
	return (p == EMPTY || p == BARS || p == BARS_FLOOR || p == BARS_CEILING);
}
