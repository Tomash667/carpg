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
	Int2 GetConnectingTile(int pokoj1, int pokoj2);

private:
	enum DODAJ
	{
		NIE_DODAWAJ,
		DODAJ_POKOJ,
		DODAJ_KORYTARZ,
		DODAJ_RAMPA
	};
	
	void GenerateInternal();
	void SetPattern();
	Room* AddRoom(int parent_room, int x, int y, int w, int h, DODAJ dodaj, int id = -1);
	void AddRoom(const Room& pokoj, int id = -1) { AddRoom(-1, pokoj.pos.x, pokoj.pos.y, pokoj.size.x, pokoj.size.y, NIE_DODAWAJ, id); }
	DIR IsFreeWay(int id);
	int CanCreateRamp(int room_index);
	Room* CreateRamp(int parent_room, const Int2& _pt, DIR dir, bool up);
	Room* CreateCorridor(int parent_room, Int2& _pt, DIR dir);
	bool CanCreateRoom(int x, int y, int w, int h);
	void SearchForConnection(int x, int y, int id);
	void SearchForConnection(Room& room, int id);
	void JoinCorridors();
	void MarkCorridors();
	void JoinRooms();
	bool IsConnectingWall(int x, int y, int id1, int id2);
	bool GenerateStairs();
	bool AddStairs(vector<Room*>& rooms, OpcjeMapy::GDZIE_SCHODY gdzie, Room*& room, Int2& pozycja, int& kierunek, bool gora, bool& w_scianie);
	bool CreateStairs(Room& room, Int2& pozycja, int& kierunek, POLE schody, bool& w_scianie);

	vector<int> wolne;
};
