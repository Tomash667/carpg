// bazowy u¿ywalny obiekt
#pragma once

//-----------------------------------------------------------------------------
#include "Resource.h"
#include "BaseObject.h"

//-----------------------------------------------------------------------------
struct Item;

//-----------------------------------------------------------------------------
struct BaseUsable : public BaseObject
{
	enum Flags
	{
		ALLOW_USE = 1 << 0,
		SLOW_STAMINA_RESTORE = 1 << 1,
		CONTAINER = 1 << 2,
		IS_BENCH = 1 << 3, // hardcoded to use variant in city
	};

	string anim, item_id, sound_id, name;
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
	int use_flags;
	ResourceState state;

	BaseUsable() : sound_timer(0), item(nullptr), sound(nullptr), limit_rot(0), use_flags(0), state(ResourceState::NotLoaded)
	{
	}

	BaseUsable& operator = (BaseObject& o)
	{
		mesh_id = o.mesh_id;
		type = o.type;
		r = o.r;
		h = o.h;
		centery = o.centery;
		flags = o.flags;
		alpha = o.alpha;
		variants = o.variants;
		extra_dist = o.extra_dist;
		return *this;
	}

	BaseUsable& operator = (BaseUsable& u)
	{
		*this = (BaseObject&)u;
		anim = u.anim;
		item_id = u.item_id;
		sound_id = u.sound_id;
		sound_timer = u.sound_timer;
		limit_rot = u.limit_rot;
		use_flags = u.use_flags;
		return *this;
	}

	bool IsContainer() const
	{
		return IS_SET(use_flags, CONTAINER);
	}

	static vector<BaseUsable*> usables;
	static BaseUsable* TryGet(const AnyString& id);
	static BaseUsable* Get(const AnyString& id)
	{
		auto usable = TryGet(id);
		assert(usable);
		return usable;
	}
};
