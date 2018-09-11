#pragma once

//-----------------------------------------------------------------------------
typedef bool (Level::*CollisionCheck)(const CollisionObject& co, const Vec3& pos, float radius) const;
typedef bool (Level::*CollisionCheckRect)(const CollisionObject& co, const Box2d& box) const;

//-----------------------------------------------------------------------------
#define CAM_COLLIDER ((void*)1)

//-----------------------------------------------------------------------------
struct CollisionObject
{
	enum Type
	{
		RECTANGLE,
		SPHERE,
		CUSTOM,
		RECTANGLE_ROT
	};

	Vec2 pt;
	union
	{
		struct
		{
			float w, h, rot, radius;
		};
		struct
		{
			CollisionCheck check;
			CollisionCheckRect check_rect;
			int extra;
		};
	};
	Type type;
	void* ptr; // w�a�ciciel tej kolizji (np Object) lub CAM_COLLIDER
};

//-----------------------------------------------------------------------------
struct CameraCollider
{
	Box box;
};
