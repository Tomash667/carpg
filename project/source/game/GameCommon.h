#pragma once

//-----------------------------------------------------------------------------
void SetGameCommonText();

//-----------------------------------------------------------------------------
enum GameDirection
{
	GDIR_DOWN,
	GDIR_LEFT,
	GDIR_UP,
	GDIR_RIGHT,
	GDIR_INVALID = -1
};

extern const Int2 g_kierunek2[4];

//-----------------------------------------------------------------------------
inline float dir_to_rot(GameDirection dir)
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
inline Int2 dir_to_pos(GameDirection dir)
{
	const Int2 k[4] = {
		Int2(0,-1),
		Int2(-1,0),
		Int2(0,1),
		Int2(1,0)
	};
	return k[dir];
}

//-----------------------------------------------------------------------------
inline Vec3 pt_to_pos(const Int2& _pt, float _y = 0)
{
	return Vec3(float(_pt.x * 2) + 1, _y, float(_pt.y * 2) + 1);
}

//-----------------------------------------------------------------------------
inline Int2 pos_to_pt(const Vec3& pos)
{
	return Int2(int(floor(pos.x / 2)), int(floor(pos.z / 2)));
}

//-----------------------------------------------------------------------------
enum Direction
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
Direction AngleToDir(float angle);
Direction GetLocationDir(const Vec2& from, const Vec2& to);
inline cstring GetLocationDirName(const Vec2& from, const Vec2& to)
{
	return kierunek_nazwa[GetLocationDir(from, to)];
}

//-----------------------------------------------------------------------------
extern vector<uint> _to_remove;
