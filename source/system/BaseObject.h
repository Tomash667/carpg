#pragma once

//-----------------------------------------------------------------------------
#include "ContentItem.h"

//-----------------------------------------------------------------------------
// Object physics type
enum OBJ_PHY_TYPE
{
	OBJ_CYLINDER,
	OBJ_HITBOX,
	OBJ_HITBOX_ROT
};

//-----------------------------------------------------------------------------
// Object flags
enum OBJ_FLAGS
{
	OBJ_NEAR_WALL = 1 << 0, // spawn object near wall
	OBJ_NO_PHYSICS = 1 << 1, // object have no physics
	OBJ_HIGH = 1 << 2, // object is placed higher (1.5 m)
	OBJ_IS_CHEST = 1 << 3, // object is chest
	OBJ_ON_WALL = 1 << 4, // object is created on wall, ignoring size
	OBJ_PRELOAD = 1 << 5, // force preload mesh
	OBJ_LIGHT = 1 << 6, // object has torch light and flame
	OBJ_TABLE_SPAWNER = 1 << 7, // generate Random table and chairs
	OBJ_CAMPFIRE_EFFECT = 1 << 8, // object has larger fire effect (requires OBJ_LIGHT)
	OBJ_IMPORTANT = 1 << 9, // try more times to generate this object
	OBJ_TMP_PHYSICS = 1 << 10, // temporary physics, only used on spawning units
	OBJ_SCALEABLE = 1 << 11, // object can be scaled, need different physics handling
	OBJ_PHYSICS_PTR = 1 << 12, // btCollisionObject user pointer points to Object
	OBJ_BUILDING = 1 << 13, // object is building
	OBJ_DOUBLE_PHYSICS = 1 << 14, // object have 2 physics colliders (only works with box for now)
	OBJ_BLOOD_EFFECT = 1 << 15, // altar blood effect
	OBJ_PHY_BLOCKS_CAM = 1 << 16, // object physics blocks camera
	OBJ_PHY_ROT = 1 << 17, // object physics can be rotated
	OBJ_WATER_EFFECT = 1 << 18, // object have water particle effect
	OBJ_MULTI_PHYSICS = 1 << 19, // object have multiple colliders (only workd with box for now)
	OBJ_CAM_COLLIDERS = 1 << 20, // spawn camera coliders from mesh attach points
	OBJ_USABLE = 1 << 21, // object is usable
	OBJ_NO_CULLING = 1 << 22, // no mesh backface culling
	OBJ_TORCH_CEILING_EFFECT = 1 << 23, // object has larger fire effect (requires OBJ_LIGHT)
};

//-----------------------------------------------------------------------------
struct VariantObject
{
	vector<Mesh*> meshes;
};

//-----------------------------------------------------------------------------
struct ObjectGroup : public ContentItem<ObjectGroup>
{
	struct EntryList
	{
		struct Entry
		{
			union
			{
				BaseObject* obj;
				EntryList* list;
			};
			uint chance;
			bool isList;

			~Entry()
			{
				if(isList)
					delete list;
			}
		};

		EntryList* parent;
		vector<Entry> entries;
		uint totalChance;

		BaseObject* GetRandom();
	};

	inline static const cstring typeName = "object group";

	EntryList list;

	BaseObject* GetRandom()
	{
		return list.GetRandom();
	}
};

//-----------------------------------------------------------------------------
// Base object
struct BaseObject : public ContentItem<BaseObject>
{
	inline static const cstring typeName = "object";

	Mesh* mesh;
	OBJ_PHY_TYPE type;
	float r, h, centery;
	Vec2 size;
	btCollisionShape* shape;
	Matrix* matrix;
	int flags;
	BaseObject* nextObj;
	VariantObject* variants;
	float extraDist; // extra distance from wall

	BaseObject() : mesh(nullptr), type(OBJ_HITBOX), centery(0), shape(nullptr), matrix(nullptr), flags(0), nextObj(nullptr),
		variants(nullptr), extraDist(0.f)
	{
	}
	virtual ~BaseObject();

	BaseObject& operator = (BaseObject& o);

	bool IsUsable() const { return IsSet(flags, OBJ_USABLE); }

	static BaseObject* TryGet(int hash, ObjectGroup** group = nullptr);
	static BaseObject* TryGet(Cstring id, ObjectGroup** group = nullptr)
	{
		return TryGet(Hash(id), group);
	}
};
