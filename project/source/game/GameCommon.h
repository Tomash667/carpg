#pragma once

//-----------------------------------------------------------------------------
void SetGameCommonText();

//-----------------------------------------------------------------------------

enum COLLISION_GROUP
{
	CG_TERRAIN = 1 << 7,
	CG_BUILDING = 1 << 8,
	CG_UNIT = 1 << 9,
	CG_OBJECT = 1 << 10,
	CG_DOOR = 1 << 11,
	CG_COLLIDER = 1 << 12,
	CG_CAMERA_COLLIDER = 1 << 13,
	CG_BARRIER = 1 << 14, // blocks only units
	 // max 1 << 15
};

//-----------------------------------------------------------------------------
enum GAME_DIR
{
	GDIR_DOWN,
	GDIR_LEFT,
	GDIR_UP,
	GDIR_RIGHT
};

//-----------------------------------------------------------------------------
inline float dir_to_rot(int dir)
{
	switch(dir)
	{
	case GDIR_DOWN:
		return 0;
	case GDIR_LEFT:
		return PI / 2;
	case GDIR_UP:
		return PI;
	case GDIR_RIGHT:
		return PI * 3 / 2;
	default:
		assert(0);
		return 0;
	}
}

//-----------------------------------------------------------------------------
inline Int2 dir_to_pos(int dir)
{
	assert(InRange(dir, 0, 4));

	const Int2 k[4] = {
		Int2(0,-1),
		Int2(-1,0),
		Int2(0,1),
		Int2(1,0)
	};

	return k[dir];
}

//-----------------------------------------------------------------------------
inline Vec3 pt_to_pos(const Int2& pt, float y = 0)
{
	return Vec3(float(pt.x * 2) + 1, y, float(pt.y * 2) + 1);
}

//-----------------------------------------------------------------------------
inline Int2 pos_to_pt(const Vec3& pos)
{
	return Int2(int(floor(pos.x / 2)), int(floor(pos.z / 2)));
}

//-----------------------------------------------------------------------------
enum KIERUNEK
{
	DIR_N,
	DIR_S,
	DIR_E,
	DIR_W,
	DIR_NE,
	DIR_NW,
	DIR_SE,
	DIR_SW
};
extern cstring kierunek_nazwa[];
extern cstring kierunek_nazwa_s[];

//-----------------------------------------------------------------------------
KIERUNEK AngleToDir(float angle);
KIERUNEK GetLocationDir(const Vec2& from, const Vec2& to);
inline cstring GetLocationDirName(const Vec2& from, const Vec2& to)
{
	return kierunek_nazwa[GetLocationDir(from, to)];
}

//-----------------------------------------------------------------------------
extern vector<uint> _to_remove;
