#pragma once

//-----------------------------------------------------------------------------
#include "Light.h"
#include "Tile.h"
#include "Room.h"

//-----------------------------------------------------------------------------
struct MapSettings
{
	enum Shape
	{
		SQUARE,
		CIRCLE
	};

	enum StairsLocation
	{
		NONE,
		RANDOM,
		FAR_FROM_ROOM
	};

	int map_w, map_h;
	Int2 room_size, corridor_size;
	int corridor_chance, corridor_join_chance, room_join_chance, bars_chance;
	Shape shape;
	StairsLocation stairs_up_loc, stairs_down_loc;
	vector<Room*>* rooms;
	Room* stairs_up_room, *stairs_down_room;
	GameDirection stairs_up_dir, stairs_down_dir;
	Int2 stairs_up_pos, stairs_down_pos;
	vector<RoomGroup>* groups;
	bool stop, stairs_down_in_wall, devmode, remove_dead_end_corridors;

	MapSettings() : stop(false)
	{
	}
};

//-----------------------------------------------------------------------------
class DungeonMapGenerator
{
public:
	void Generate(MapSettings& settings, bool recreate = false);
	void ContinueGenerating() { Finalize(); }
	Int2 GetConnectingTile(Room* room1, Room* room2);
	Tile* GetMap() { return map; }

private:
	void SetLayout();
	void GenerateRooms();
	GameDirection CheckFreePath(int id);
	void AddRoom(int x, int y, int w, int h, bool is_corridor, Room* parent);
	void SetRoom(Room* room);
	void AddCorridor(Int2& pt, GameDirection dir, Room* parent);
	bool CheckRoom(int x, int y, int w, int h);
	void SetWall(TILE_TYPE& tile);
	void RemoveDeadEndCorridors();
	void RemoveWall(int x, int y, Room* room);
	void SetRoomIndices();
	void MarkCorridors();
	void JoinCorridors();
	void AddHoles();
	void Finalize();
	void JoinRooms();
	bool IsConnectingWall(int x, int y, Room* room1, Room* room2);
	void CreateRoomGroups();
	void DrawRoomGroups();
	void GenerateStairs();
	void GenerateStairs(vector<Room*>& rooms, MapSettings::StairsLocation loc, Room*& room, Int2& pos, GameDirection& dir, bool up, bool& in_wall);
	bool AddStairs(Room& room, Int2& pos, GameDirection& dir, TILE_TYPE tile, bool& in_wall);
	void SetRoomGroupTargets();

	Tile* map;
	MapSettings* settings;
	vector<pair<int, Room*>> empty;
	vector<pair<Room*, Room*>> joined;
	vector<Room*> map_rooms;
	int map_w, map_h;
};
