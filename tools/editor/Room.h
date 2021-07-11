#pragma once

struct Room
{
	Vec3 GetPoint(Dir dir) const;

	Box box;
};
