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
	GDIR_INVALID
};

//-----------------------------------------------------------------------------
enum GameDirectionFlag
{
	GDIRF_DOWN = 1 << 0,
	GDIRF_LEFT = 1 << 1,
	GDIRF_UP = 1 << 2,
	GDIRF_RIGHT = 1 << 3,
	GDIRF_LEFT_DOWN = 1 << 4,
	GDIRF_LEFT_UP = 1 << 5,
	GDIRF_RIGHT_DOWN = 1 << 6,
	GDIRF_RIGHT_UP = 1 << 7,
	GDIRF_DOWN_ROW = GDIRF_DOWN | GDIRF_LEFT_DOWN | GDIRF_RIGHT_DOWN,
	GDIRF_UP_ROW = GDIRF_UP | GDIRF_LEFT_UP | GDIRF_RIGHT_UP,
	GDIRF_LEFT_ROW = GDIRF_LEFT | GDIRF_LEFT_DOWN | GDIRF_LEFT_UP,
	GDIRF_RIGHT_ROW = GDIRF_RIGHT | GDIRF_RIGHT_DOWN | GDIRF_RIGHT_UP
};

//-----------------------------------------------------------------------------
inline constexpr GameDirection Reversed(GameDirection dir)
{
	switch(dir)
	{
	case GDIR_DOWN:
		return GDIR_UP;
	case GDIR_LEFT:
		return GDIR_RIGHT;
	case GDIR_UP:
		return GDIR_DOWN;
	case GDIR_RIGHT:
		return GDIR_LEFT;
	default:
		assert(0);
		return GDIR_DOWN;
	}
}

//-----------------------------------------------------------------------------
inline constexpr float DirToRot(GameDirection dir)
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
inline Int2 DirToPos(GameDirection dir)
{
	const Int2 k[5] = {
		Int2(0,-1),
		Int2(-1,0),
		Int2(0,1),
		Int2(1,0),
		Int2(0,0)
	};
	return k[dir];
}

//-----------------------------------------------------------------------------
inline Vec3 PtToPos(const Int2& pt, float y = 0)
{
	return Vec3(float(pt.x * 2) + 1, y, float(pt.y * 2) + 1);
}

//-----------------------------------------------------------------------------
inline Int2 PosToPt(const Vec3& pos)
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
extern cstring dir_name[];
extern cstring dir_name_short[];

//-----------------------------------------------------------------------------
Direction AngleToDir(float angle);
Direction GetLocationDir(const Vec2& from, const Vec2& to);
inline cstring GetLocationDirName(const Vec2& from, const Vec2& to)
{
	return dir_name[GetLocationDir(from, to)];
}
inline cstring GetLocationDirName(float angle)
{
	return dir_name[AngleToDir(angle)];
}

//-----------------------------------------------------------------------------
inline float ConvertOldAngleToNew(float angle)
{
	return Clip(angle + PI / 2);
}
inline float ConvertNewAngleToOld(float angle)
{
	return Clip(angle - PI / 2);
}

//-----------------------------------------------------------------------------
extern vector<uint> _to_remove;
