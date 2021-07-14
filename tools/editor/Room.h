#pragma once

struct Link
{
	Room* room;
	Dir dir;
};

struct Room
{
	Vec3 GetPoint(Dir dir) const;

	Box box;
	vector<Link> links;
};
