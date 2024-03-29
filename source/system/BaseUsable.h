// bazowy u�ywalny obiekt
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
		ALCHEMY = 1 << 4,
		DESTROYABLE = 1 << 5, // can be destroyed with attack
		RESISTANT = 1 << 6, // use with DESTROYABLE, can be destroyed by script
	};

	string anim, name;
	float soundTimer;
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
	int limitRot;
	int useFlags;
	ResourceState state;

	BaseUsable() : soundTimer(0), item(nullptr), sound(nullptr), limitRot(0), useFlags(0), state(ResourceState::NotLoaded)
	{
	}

	BaseUsable& operator = (BaseObject& o);
	BaseUsable& operator = (BaseUsable& u);

	bool IsContainer() const { return IsSet(useFlags, CONTAINER); }
	void EnsureIsLoaded();

	static vector<BaseUsable*> usables;
	static BaseUsable* TryGet(int hash)
	{
		BaseObject* obj = BaseObject::TryGet(hash);
		if(obj && obj->IsUsable())
			return static_cast<BaseUsable*>(obj);
		return nullptr;
	}
	static BaseUsable* TryGet(Cstring id)
	{
		return TryGet(Hash(id));
	}
	static BaseUsable* Get(int hash)
	{
		BaseUsable* use = TryGet(hash);
		if(use)
			return use;
		throw Format("Missing usable hash %d.", hash);
	}
	static BaseUsable* Get(Cstring id)
	{
		BaseUsable* use = TryGet(id);
		if(use)
			return use;
		throw Format("Missing usable '%s'.", id);
	}
};
