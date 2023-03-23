#include "Pch.h"
#include "BaseUsable.h"

#include <Mesh.h>
#include <ResourceManager.h>

//-----------------------------------------------------------------------------
vector<BaseUsable*> BaseUsable::usables;
Ptr<BaseUsable> BaseUsable::chair, BaseUsable::goldVein, BaseUsable::ironVein, BaseUsable::stool, BaseUsable::throne;

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
void BaseUsable::EnsureIsLoaded()
{
	if(state == ResourceState::NotLoaded)
	{
		if(variants)
		{
			for(Mesh* mesh : variants->meshes)
				resMgr->Load(mesh);
		}
		else
			resMgr->Load(mesh);
		if(sound)
			resMgr->Load(sound);
		state = ResourceState::Loaded;
	}
}
