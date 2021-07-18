#pragma once

struct RoomLink
{
	Room* room;
	Dir dir;
	Box box;
	bool first;
};

struct Room
{
	Vec3 GetPoint(Dir dir) const;

	Box box;
	vector<RoomLink> links;
};
