// bazowy u¿ywalny obiekt
#pragma once

//-----------------------------------------------------------------------------
#include "Resource.h"

//-----------------------------------------------------------------------------
struct Item;
struct BaseObject;

//-----------------------------------------------------------------------------
enum USEABLE_ID
{
	U_CHAIR,
	U_BENCH,
	U_ANVIL,
	U_CAULDRON,
	U_IRON_VEIN,
	U_GOLD_VEIN,
	U_THRONE,
	U_STOOL,
	U_BENCH_ROT,
	U_BOOKSHELF,
	U_MAX
};

//-----------------------------------------------------------------------------
struct BaseUsable
{
	enum Flags
	{
		ALLOW_USE = 1 << 0,
		SLOW_STAMINA_RESTORE = 1 << 1,
		CONTAINER = 1 << 2
	};

	cstring id, name, obj_name, anim, item_id, sound_id;
	float sound_timer;
	const Item* item;
	SoundPtr sound;
	/* For container:
		0 - no limit
		1 - 90* angle
		2 - 45* angle
	For not container:
		0 - use player rotation
		1 - can be used from one of two sides (left/right)
		2 - use object rotation
		3 - can be used from one of two sides (front/back)
		4 - use object rotation, can be used from 90* angle
	*/
	int limit_rot;
	int flags;
	BaseObject* obj;
	ResourceState state;

	BaseUsable(cstring id, cstring obj_name, cstring anim, int limit_rot, int flags, cstring item_id = nullptr,
		cstring sound_id = nullptr, float sound_timer = 0.f) :
		id(id), name(nullptr), anim(anim), item_id(item_id), sound_id(sound_id), sound_timer(sound_timer), obj_name(obj_name), item(nullptr), sound(nullptr),
		limit_rot(limit_rot), flags(flags), obj(nullptr), state(ResourceState::NotLoaded)
	{
	}
};
extern BaseUsable g_base_usables[];
extern const uint n_base_usables;
