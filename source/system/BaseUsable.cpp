#include "Pch.h"
#include "BaseUsable.h"

//-----------------------------------------------------------------------------
vector<BaseUsable*> BaseUsable::usables;

//=================================================================================================
BaseUsable& BaseUsable::operator = (BaseObject& o)
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

//=================================================================================================
BaseUsable& BaseUsable::operator = (BaseUsable& u)
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

//=================================================================================================
/*BaseUsable* BaseUsable::TryGet(Cstring id)
{
	for(auto use : usables)
	{
		if(use->id == id)
			return use;
	}

	return nullptr;
}*/
