#pragma once

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
	float rot;
	Type type;
};
