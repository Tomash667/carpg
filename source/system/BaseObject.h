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
	OBJ_BILLBOARD = 1 << 10, // object always face camera
	OBJ_SCALEABLE = 1 << 11, // object can be scaled, need different physics handling
	OBJ_PHYSICS_PTR = 1 << 12, // btCollisionObject user pointer points to Object (do not use in new code, only used in tutorial, don't support save & load)
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
			bool is_list;

			~Entry()
			{
				if(is_list)
					delete list;
			}
		};

		EntryList* parent;
		vector<Entry> entries;
		uint total_chance;

		BaseObject* GetRandom();
	};

	inline static const cstring type_name = "object group";

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
	inline static const cstring type_name = "object";

	Mesh* mesh;
	OBJ_PHY_TYPE type;
	float r, h, centery;
	Vec2 size;
	btCollisionShape* shape;
	Matrix* matrix;
	int flags;
	BaseObject* next_obj;
	VariantObject* variants;
	float extra_dist; // extra distance from wall

	BaseObject() : mesh(nullptr), type(OBJ_HITBOX), centery(0), shape(nullptr), matrix(nullptr), flags(0), next_obj(nullptr),
		variants(nullptr), extra_dist(0.f)
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

	/*static SetContainer<BaseObject> objs;
	static BaseObject* TryGet(Cstring id, ObjectGroup** group = nullptr);
	static BaseObject* Get(Cstring id, ObjectGroup** group = nullptr)
	{
		BaseObject* obj = TryGet(id, group);
		assert(obj && "Missing BaseObject!");
		return obj;
	}
	static BaseObject* GetS(const string& id)
	{
		return Get(id);
	}*/
};
