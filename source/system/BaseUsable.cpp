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
	extraDist = o.extraDist;
	return *this;
}

//=================================================================================================
BaseUsable& BaseUsable::operator = (BaseUsable& u)
{
	*this = (BaseObject&)u;
	anim = u.anim;
	item = u.item;
	sound = u.sound;
	soundTimer = u.soundTimer;
	limitRot = u.limitRot;
	useFlags = u.useFlags;
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
