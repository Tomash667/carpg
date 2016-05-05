#pragma once

enum ROT_HINT
{
	BOTTOM,
	LEFT,
	TOP,
	RIGHT,
	RANDOM // to remove
};

struct Room2;

struct RoomDoor
{
	Room2* room;
	VEC3 pos;
	float rot;
	int index;

	inline RoomDoor(Room2* room, const VEC3& pos, float rot, int index) : room(room), pos(pos), rot(rot), index(index) {}
};

struct Room2
{
	enum Type
	{
		CORRIDOR,
		ROOM,
		JUNCTION
	};

	vector<uint> connected;
	VEC3 pos, size;
	INT2 tile_size;
	float rot;
	Type type;
	int index, holes;
	btCollisionObject* cobj;

	inline RoomDoor GetRoomDoor(ROT_HINT r)
	{
		switch(r)
		{
		default:
		case LEFT:
			return RoomDoor(this, VEC3(pos.x - size.x / 2, 0, pos.z), PI, LEFT);
		case RIGHT:
			return RoomDoor(this, VEC3(pos.x + size.x / 2, 0, pos.z), 0, RIGHT);
		case TOP:
			return RoomDoor(this, VEC3(pos.x, 0, pos.z + size.z / 2), PI * 3 / 2, TOP);
		case BOTTOM:
			return RoomDoor(this, VEC3(pos.x, 0, pos.z - size.z / 2), PI / 2, BOTTOM);
		}
	}
};
