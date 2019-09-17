#pragma once

//-----------------------------------------------------------------------------
#include "Tile.h"
#include "Room.h"
#include "Unit.h"
#include "Chest.h"
#include "Trap.h"
#include "Door.h"
#include "GroundItem.h"
#include "Usable.h"
#include "Object.h"
#include "GameCommon.h"
#include "LevelArea.h"

//-----------------------------------------------------------------------------
struct InsideLocationLevel : public LevelArea
{
	Tile* map;
	int w, h;
	vector<Room*> rooms;
	vector<RoomGroup> groups;
	Int2 staircase_up, staircase_down;
	GameDirection staircase_up_dir, staircase_down_dir;
	bool staircase_down_in_wall;

	InsideLocationLevel(int area_id) : LevelArea(LevelArea::Type::Inside, area_id, false), map(nullptr) {}
	~InsideLocationLevel();

	bool IsInside(int x, int y) const { return x >= 0 && y >= 0 && x < w && y < h; }
	bool IsInside(const Int2& pt) const { return IsInside(pt.x, pt.y); }
	Vec3 GetRandomPos() const { return Vec3(Random(2.f*w), 0, Random(2.f*h)); }
	Room* GetNearestRoom(const Vec3& pos);
	Room* FindEscapeRoom(const Vec3& my_pos, const Vec3& enemy_pos);
	Int2 GetUpStairsFrontTile() const { return staircase_up + DirToPos(staircase_up_dir); }
	Int2 GetDownStairsFrontTile() const { return staircase_down + DirToPos(staircase_down_dir); }
	Room* GetRandomRoom() { return rooms[Rand() % rooms.size()]; }
	bool GetRandomNearWallTile(const Room& room, Int2& tile, GameDirection& rot, bool nocol = false);
	Room& GetFarRoom(bool have_down_stairs, bool no_target = false);
	Room* GetRoom(const Int2& pt);
	Room* GetUpStairsRoom() { return GetRoom(staircase_up); }
	Room* GetDownStairsRoom() { return GetRoom(staircase_down); }
	Room* GetRandomRoom(RoomTarget target, delegate<bool(Room&)> clbk, int* index, int* group);
	pair<Room*, Room*> GetConnectingRooms(RoomGroup* group1, RoomGroup* group2);

	void SaveLevel(GameWriter& f, bool local);
	void LoadLevel(GameReader& f, bool local);

	Tile& At(const Int2& pt)
	{
		assert(IsInside(pt));
		return map[pt(w)];
	}
	Door* FindDoor(const Int2& pt) const;

	bool IsTileVisible(const Vec3& pos) const
	{
		Int2 pt = PosToPt(pos);
		return IsSet(map[pt(w)].flags, Tile::F_REVEALED);
	}

	bool IsValidWalkPos(const Vec3& pos, float radius) const
	{
		return !(pos.x < 2.f + radius || pos.y < 2.f + radius || pos.x > 2.f*w - 2.f - radius || pos.y > 2.f*h - 2.f - radius);
	}

	// sprawdza czy pole le¿y przy œcianie, nie uwzglêdnia na ukos, nie mo¿e byæ na krawêdzi mapy!
	bool IsTileNearWall(const Int2& pt) const;
	bool IsTileNearWall(const Int2& pt, int& dir) const;

	Room& GetRoom(RoomTarget target, bool down_stairs);
	int GetTileDirFlags(const Int2& pt);
};
