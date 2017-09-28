#pragma once

//-----------------------------------------------------------------------------
struct Mesh;
enum USABLE_ID;

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
	OBJ_CHEST = 1 << 3, // object is chest TO REMOVE
	OBJ_ON_WALL = 1 << 4, // object is created on wall, ignoring size
	OBJ_PRELOAD = 1 << 5, // force preload mesh
	OBJ_LIGHT = 1 << 6, // object has torch light and flame
	OBJ_TABLE = 1 << 7, // generate Random table and chairs
	OBJ_CAMPFIRE = 1 << 8, // object has larger fire effect (requires OBJ_LIGHT)
	OBJ_IMPORTANT = 1 << 9, // try more times to generate this object
	OBJ_BILLBOARD = 1 << 10, // object always face camera
	OBJ_USABLE = 1 << 11, // object is usable TO REMOVE
	OBJ_BENCH = 1 << 12, // object is bench TO REMOVE
	OBJ_ANVIL = 1 << 13, // object is anvil TO REMOVE
	OBJ_CHAIR = 1 << 14, // object is chair TO REMOVE
	OBJ_CAULDRON = 1 << 15, // object is cauldron TO REMOVE
	OBJ_SCALEABLE = 1 << 16, // object can be scaled, need different physics handling
	OBJ_PHYSICS_PTR = 1 << 17, // set Object::ptr, in bullet object set pointer to Object
	OBJ_BUILDING = 1 << 18, // object is building
	OBJ_DOUBLE_PHYSICS = 1 << 19, // object have 2 physics colliders (only works with box for now)
	OBJ_BLOOD_EFFECT = 1 << 20, // altar blood effect
	OBJ_REQUIRED = 1 << 21, // object is required to generate (if not will throw exception)
	OBJ_IN_MIDDLE = 1 << 22, // object is generated in midle of room
	OBJ_THRONE = 1 << 23, // object is throne TO REMOVE
	OBJ_IRON_VEIN = 1 << 24, // object is iron vein TO REMOVE
	OBJ_GOLD_VEIN = 1 << 25, // object is gold vein TO REMOVE
	OBJ_BOOKSHELF = 1 << 26, // object is bookshelf TO REMOVE
	OBJ_PHY_BLOCKS_CAM = 1 << 27, // object physics blocks camera
	OBJ_PHY_ROT = 1 << 28, // object physics can be rotated
	OBJ_WATER_EFFECT = 1 << 29, // object have water particle effect
	// unused 1<<30
	OBJ_STOOL = 1 << 31, // object is stool TO REMOVE
};
enum OBJ_FLAGS2
{
	OBJ2_BENCH_ROT = 1 << 0, // object is rotated bech (can be only used from correct side) TO REMOVE
	OBJ2_VARIANT = 1 << 1, // object have multiple mesh variants (must be usable with box physics for now)
	OBJ2_MULTI_PHYSICS = 1 << 2, // object have multiple colliders (only workd with box for now)
	OBJ2_CAM_COLLIDERS = 1 << 3, // spawn camera coliders from mesh attach points
};

//-----------------------------------------------------------------------------
struct VariantObject
{
	struct Entry
	{
		string mesh_id;
		Mesh* mesh;
	};
	vector<Entry> entries;
	bool loaded;
};

//-----------------------------------------------------------------------------
// Base object
struct BaseObject
{
	string id; mesh_id;
	Mesh* mesh;
	OBJ_PHY_TYPE type;
	float r, h, centery;
	Vec2 size;
	btCollisionShape* shape;
	Matrix* matrix;
	int flags, flags2, alpha;
	BaseObject* next_obj;
	VariantObject* variant;
	float extra_dist; // extra distance from wall

	BaseObject() : mesh(nullptr), type(OBJ_HITBOX), centery(0), shape(nullptr), matrix(nullptr), flags(0), flags2(0), alpha(-1), next_obj(nullptr),
		variant(nullptr), extra_dist(nullptr)
	{
	}

	BaseObject& operator = (BaseObject& o)
	{
		mesh_id = o.mesh_id;
		type = o.type;
		r = o.r;
		h = o.h;
		centery = o.centery;
		flags = o.flags;
		flags2 = o.flags2;
		alpha = o.alpha;
		variant = o.variant;
		extra_dist = o.extra_dist;
	}

	USABLE_ID ToUsableType() const;

	// is_variant is set when there is multiple variants
	static BaseObject* TryGet(cstring id, bool* is_variant = nullptr);
	static BaseObject* Get(cstring id, bool* is_variant = nullptr)
	{
		BaseObject* obj = TryGet(id, is_variant);
		assert(obj && "Missing BaseObject!");
		return obj;
	}
	static cstring GetRandomPainting();

	static BaseObject objs[];
	static const uint n_objs;
};
