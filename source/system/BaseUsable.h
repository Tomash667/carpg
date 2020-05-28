// bazowy u¿ywalny obiekt
#pragma once

//-----------------------------------------------------------------------------
#include "BaseObject.h"

//-----------------------------------------------------------------------------
struct BaseUsable : public BaseObject
{
	enum Flags
	{
		ALLOW_USE_ITEM = 1 << 0,
		SLOW_STAMINA_RESTORE = 1 << 1,
		CONTAINER = 1 << 2,
		IS_BENCH = 1 << 3, // hardcoded to use variant in city
		ALCHEMY = 1 << 4
	};

	string anim, name;
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
		mesh = o.mesh;
		type = o.type;
		r = o.r;
		h = o.h;
		centery = o.centery;
		flags = o.flags;
		variants = o.variants;
		extra_dist = o.extra_dist;
		return *this;
	}

	BaseUsable& operator = (BaseUsable& u)
	{
		*this = (BaseObject&)u;
		anim = u.anim;
		item = u.item;
		sound = u.sound;
		sound_timer = u.sound_timer;
		limit_rot = u.limit_rot;
		use_flags = u.use_flags;
		return *this;
	}

	bool IsContainer() const
	{
		return IsSet(use_flags, CONTAINER);
	}

	static vector<BaseUsable*> usables;
	static BaseUsable* TryGet(Cstring id);
	static BaseUsable* Get(Cstring id)
	{
		auto usable = TryGet(id);
		assert(usable);
		return usable;
	}
};
