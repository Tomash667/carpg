#pragma once

//-----------------------------------------------------------------------------
enum COLLISION_GROUP
{
	CG_TERRAIN = 1 << 7,
	CG_BUILDING = 1 << 8,
	CG_UNIT = 1 << 9,
	CG_OBJECT = 1 << 10,
	CG_DOOR = 1 << 11,
	CG_COLLIDER = 1 << 12,
	CG_CAMERA_COLLIDER = 1 << 13,
	CG_BARRIER = 1 << 14, // blocks only units
	// max 1 << 15
};

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
	void* ptr; // w³aœciciel tej kolizji (np Object) lub CAM_COLLIDER
};

//-----------------------------------------------------------------------------
struct CameraCollider
{
	Box box;
};
