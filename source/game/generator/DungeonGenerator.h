#pragma once

#include "Room.h"

class InsideLocation2;

class DungeonGenerator
{
	const Config* cfg;
	Location2* loc;
	vector<Room2*> rooms;
	btCollisionWorld* world;
	vector<btCollisionShape*> shapes;
	int order[4];
	vector<RoomPortal> doors;
	vector<RoomPortal> backup_doors;

	bool CreateRoomPhy(Room2* room, btCollisionObject* other_cobj, float y = 0.f);
	bool GenerateStep();

public:
	struct Config
	{
		int max_rooms;
		int corridor_to_corridor;
		int corridor_to_room;
		int corridor_to_junction;
		int room_to_corridor;
		int room_to_room;
		int room_to_junction;
		int junction_to_corridor;
		int junction_to_room;
		int junction_to_junction;
	};

	void Init(btCollisionWorld* world);
	void Generate(InsideLocation2* loc, const Config& cfg);
};
