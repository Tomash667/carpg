#include "Pch.h"
#include "Room.h"

Vec3 Room::GetPoint(Dir dir) const
{
	switch(dir)
	{
	default:
	case DIR_RIGHT:
		return Vec3(box.v2.x, box.v1.y, box.v1.z + (box.v2.z - box.v1.z) / 2);
	case DIR_FORWARD_RIGHT:
		return Vec3(box.v2.x, box.v1.y, box.v2.z);
	case DIR_FORWARD:
		return Vec3(box.v1.x + (box.v2.x - box.v1.x) / 2, box.v1.y, box.v2.z);
	case DIR_FORWARD_LEFT:
		return Vec3(box.v1.x, box.v1.y, box.v2.z);
	case DIR_LEFT:
		return Vec3(box.v1.x, box.v1.y, box.v1.z + (box.v2.z - box.v1.z) / 2);
	case DIR_BACKWARD_LEFT:
		return Vec3(box.v1.x, box.v1.y, box.v1.z);
	case DIR_BACKWARD:
		return Vec3(box.v1.x + (box.v2.x - box.v1.x) / 2, box.v1.y, box.v1.z);
	case DIR_BACKWARD_RIGHT:
		return Vec3(box.v2.x, box.v1.y, box.v1.z);
	}
}
