#pragma once

//-----------------------------------------------------------------------------
#include "Animesh.h"

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
	OBJ_NEAR_WALL = 1<<0, // spawn object near wall
	OBJ_NO_PHYSICS = 1<<1, // object have no physics
	OBJ_HIGH = 1<<2, // object is placed higher (1.5 m)
	OBJ_CHEST = 1<<3, // object is chest
	OBJ_ON_WALL = 1<<4, // object is created on wall, ignoring size
	// unused 1<<5
	OBJ_LIGHT = 1<<6, // object has torch light and flame
	OBJ_TABLE = 1<<7, // generate random table and chairs
	OBJ_CAMPFIRE = 1<<8, // object has larger fire effect (requires OBJ_LIGHT)
	OBJ_IMPORTANT = 1<<9, // try more times to generate this object
	OBJ_BILLBOARD = 1<<10, // object always face camera
	OBJ_USEABLE = 1<<11, // object is useable
	OBJ_BENCH = 1<<12, // object is bench
	OBJ_ANVIL = 1<<13, // object is anvil
	OBJ_CHAIR = 1<<14, // object is chair
	OBJ_CAULDRON = 1<<15, // object is cauldron
	OBJ_SCALEABLE = 1<<16, // object can be scaled, need different physics handling
	OBJ_PHYSICS_PTR = 1<<17, // set Object::ptr, in bullet object set pointer to Object
	OBJ_BUILDING = 1<<18, // object is building
	OBJ_DOUBLE_PHYSICS = 1<<19, // object have 2 physics colliders (only works with box for now)
	OBJ_BLOOD_EFFECT = 1<<20, // altar blood effect
	OBJ_REQUIRED = 1<<21, // object is required to generate (if not will throw exception)
	OBJ_IN_MIDDLE = 1<<22, // object is generated in midle of room
	OBJ_THRONE = 1<<23, // object is throne
	OBJ_IRON_VAIN = 1<<24, // object is iron vain
	OBJ_GOLD_VAIN = 1<<25, // object is gold vain
	OBJ_V0_CONVERSION = 1<<26, // convert vain objects to useables
	OBJ_PHY_BLOCKS_CAM = 1<<27, // object physics blocks camera
	OBJ_PHY_ROT = 1<<28, // object physics can be rotated
	OBJ_WATER_EFFECT = 1<<29, // object have water particle effect
	// unused 1<<30
	OBJ_STOOL = 1<<31, // object is stool
};
enum OBJ_FLAGS2
{
	OBJ2_BENCH_ROT = 1<<0, // object is rotated bech (can be only used from correct side)
	OBJ2_VARIANT = 1<<1, // object have multiple mesh variants (must be useable with box physics for now)
	OBJ2_MULTI_PHYSICS = 1<<2, // object have multiple colliders (only workd with box for now)
	OBJ2_CAM_COLLIDERS = 1<<3, // spawn camera coliders from mesh attach points
};

//-----------------------------------------------------------------------------
struct VariantObj
{
	struct Entry
	{
		cstring mesh_name;
		Animesh* mesh;
	};
	Entry* entries;
	uint count;
	bool loaded;
};

//-----------------------------------------------------------------------------
// Base object
struct Obj
{
	cstring id, name, mesh_id;
	Animesh* mesh;
	OBJ_PHY_TYPE type;
	float r, h, centery;
	VEC2 size;
	btCollisionShape* shape;
	MATRIX* matrix;
	int flags, flags2, alpha;
	Obj* next_obj;
	VariantObj* variant;
	float extra_dist;

	Obj() : flags(0), flags2(0)
	{

	}

	Obj(cstring id, int flags, int flags2, cstring name, cstring mesh_id, int alpha=-1, float centery=0.f, VariantObj* variant=nullptr, float extra_dist=0.f) :
		id(id), name(name), mesh_id(mesh_id), type(OBJ_HITBOX), mesh(nullptr), shape(nullptr), matrix(nullptr), flags(flags), flags2(flags2), alpha(alpha),
		centery(centery), next_obj(nullptr), variant(variant), extra_dist(extra_dist)
	{

	}

	Obj(cstring id, int flags, int flags2, cstring name, cstring mesh_id, float radius, float height, int alpha=-1, float centery=0.f,
		VariantObj* variant=nullptr, float extra_dist=0.f) :
		id(id), name(name), mesh_id(mesh_id), type(OBJ_CYLINDER), mesh(nullptr), shape(nullptr), r(radius), h(height), matrix(nullptr), flags(flags),
		flags2(flags2), alpha(alpha), centery(centery), next_obj(nullptr), variant(variant), extra_dist(extra_dist)
	{

	}
};
extern Obj g_objs[];
extern const uint n_objs;

//-----------------------------------------------------------------------------
inline cstring GetRandomPainting()
{
	if(rand2()%100 == 0)
		return "painting3";
	switch(rand2()%23)
	{
	case 0:
		return "painting1";
	case 1:
	case 2:
		return "painting2";
	case 3:
	case 4:
		return "painting4";
	case 5:
	case 6:
		return "painting5";
	case 7:
	case 8:
		return "painting6";
	case 9:
		return "painting7";
	case 10:
		return "painting8";
	case 11:
	case 12:
	case 13:
		return "painting_x1";
	case 14:
	case 15:
	case 16:
		return "painting_x2";
	case 17:
	case 18:
	case 19:
		return "painting_x3";
	case 20:
	case 21:
	case 22:
	default:
		return "painting_x4";
	}
}

//-----------------------------------------------------------------------------
// szuka obiektu, is_variant ustawia tylko na true jeœli ma kilka wariantów
inline Obj* FindObjectTry(cstring _id, bool* is_variant=nullptr)
{
	assert(_id);

	if(strcmp(_id, "painting") == 0)
	{
		if(is_variant)
			*is_variant = true;
		return FindObjectTry(GetRandomPainting());
	}

	if(strcmp(_id, "tombstone") == 0)
	{
		if(is_variant)
			*is_variant = true;
		int id = random(0,9);
		if(id != 0)
			return FindObjectTry(Format("tombstone_x%d", id));
		else
			return FindObjectTry("tombstone_1");
	}

	if(strcmp(_id, "random") == 0)
	{
		switch(rand2()%3)
		{
		case 0: return FindObjectTry("wheel");
		case 1: return FindObjectTry("rope");
		case 2: return FindObjectTry("woodpile");
		}
	}

	for(uint i=0; i<n_objs; ++i)
	{
		if(strcmp(g_objs[i].id, _id) == 0)
			return &g_objs[i];
	}

	return nullptr;
}

//-----------------------------------------------------------------------------
inline Obj* FindObject(cstring _id, bool* is_variant=nullptr)
{
	Obj* obj = FindObjectTry(_id, is_variant);
	assert(obj && "Brak takiego obiektu!");
	return obj;
}


//-----------------------------------------------------------------------------
// Object in game
struct Object
{
	VEC3 pos, rot;
	float scale;
	Animesh* mesh;
	Obj* base;
	void* ptr;
	bool require_split;

	static const int MIN_SIZE = 29;

	Object() : require_split(false)
	{

	}

	inline float GetRadius() const
	{
		return mesh->head.radius * scale;
	}
	// -1 - nie, 0 - tak, 1 - tak i bez cullingu
	inline int RequireAlphaTest() const
	{
		if(!base)
			return -1;
		else
			return base->alpha;
	}
	inline bool IsBillboard() const
	{
		return base && IS_SET(base->flags, OBJ_BILLBOARD);
	}
	void Save(HANDLE file);
	// zwraca false jeœli obiekt trzeba usun¹æ i zast¹piæ czymœ innym
	// aktualnie obs³ugiwane tylko przez InsideLocationLevel
	bool Load(HANDLE file);
	void Swap(Object& o);
	void Write(BitStream& stream) const;
	bool Read(BitStream& stream);
};
