#pragma once

#include "MapGenerator.h"

//-----------------------------------------------------------------------------
struct DungeonGenerator : MapGenerator
{
	enum DIR
	{
		DOL,
		LEWO,
		GORA,
		PRAWO,
		BRAK
	};

	bool Generate(OpcjeMapy& opcje, bool recreate = false);
	bool ContinueGenerate();
	Int2 GetConnectingTile(int room1, int room2);

private:
	enum ADD
	{
		ADD_NO,
		ADD_ROOM,
		ADD_CORRIDOR,
		ADD_RAMP
	};
	
	void GenerateInternal();
	void SetPattern();
	Room* AddRoom(int parent_room, int x, int y, int w, int h, ADD add, int id = -1);
	void AddRoom(const Room& room, int id) { AddRoom(-1, room.pos.x, room.pos.y, room.size.x, room.size.y, ADD_NO, id); }
	DIR IsFreeWay(int id);
	int CanCreateRamp(int room_index);
	Room* CreateRamp(int parent_room, const Int2& pt, DIR dir, bool up);
	Room* CreateCorridor(int parent_room, Int2& pt, DIR dir);
	bool CanCreateRoom(int x, int y, int w, int h);
	void SearchForConnection(int x, int y, int id);
	void SearchForConnection(Room& room, int id);
	void JoinCorridors();
	void MarkCorridors();
	void JoinRooms();
	bool IsConnectingWall(int x, int y, int id1, int id2);
	bool GenerateStairs();
	bool AddStairs(vector<Room*>& rooms, OpcjeMapy::GDZIE_SCHODY where, Room*& room, Int2& pt, int& dir, bool up, bool& inside_wall);
	bool CreateStairs(Room& room, Int2& pt, int& dir, POLE stairs_type, bool& inside_wall);

	vector<int> free;
};
