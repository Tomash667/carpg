#pragma once

//-----------------------------------------------------------------------------
#include "Tile.h"
#include "Room.h"

//-----------------------------------------------------------------------------
struct MapSettings
{
	enum Shape
	{
		SHAPE_SQUARE,
		SHAPE_CIRCLE
	};

	enum EntryLocation
	{
		ENTRY_NONE,
		ENTRY_RANDOM,
		ENTRY_FAR_FROM_ROOM,
		ENTRY_BORDER,
		ENTRY_FAR_FROM_PREV
	};

	int mapW, mapH;
	Int2 roomSize, corridorSize;
	int corridorChance, corridorJoinChance, roomJoinChance, barsChance;
	Shape shape;
	EntryType prevEntryType, nextEntryType;
	EntryLocation prevEntryLoc, nextEntryLoc;
	vector<Room*>* rooms;
	Room* prevEntryRoom, *nextEntryRoom;
	GameDirection prevEntryDir, nextEntryDir;
	Int2 prevEntryPt, nextEntryPt;
	vector<RoomGroup>* groups;
	bool stop, devmode, removeDeadEndCorridors;

	MapSettings() : stop(false), prevEntryPt(-1000, -1000), nextEntryPt(-1000, -1000)
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
	void AddRoom(int x, int y, int w, int h, bool isCorridor, Room* parent);
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
	void GenerateEntry();
	void GenerateEntry(vector<Room*>& rooms, MapSettings::EntryLocation loc, EntryType& type, Room*& room, Int2& pt, GameDirection& dir, bool isPrev);
	bool AddEntryFarFromPoint(vector<Room*>& rooms, const Int2& farPt, Room*& room, EntryType& type, Int2& pt, GameDirection& dir, bool isPrev);
	bool AddEntry(Room& room, EntryType& type, Int2& pt, GameDirection& dir, bool isPrev);
	void SetRoomGroupTargets();

	Tile* map;
	MapSettings* settings;
	vector<pair<int, Room*>> empty;
	vector<pair<Room*, Room*>> joined;
	vector<Room*> mapRooms;
	int mapW, mapH;
};
