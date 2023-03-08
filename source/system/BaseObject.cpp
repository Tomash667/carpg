#include "Pch.h"
#include "BaseObject.h"

//=================================================================================================
BaseObject* ObjectGroup::EntryList::GetRandom()
{
	EntryList* list = this;
	while(true)
	{
		uint c = Rand() % list->totalChance;
		uint total = 0u;
		for(auto& e : list->entries)
		{
			total += e.chance;
			if(c <= total)
			{
				if(e.isList)
				{
					list = e.list;
					break;
				}
				else
					return e.obj;
			}
		}
	}
}

//=================================================================================================
BaseObject::~BaseObject()
{
	delete shape;
	if(IsSet(flags, OBJ_DOUBLE_PHYSICS))
		delete nextObj;
	else if(IsSet(flags, OBJ_MULTI_PHYSICS))
		delete[] nextObj;
}

//=================================================================================================
BaseObject& BaseObject::operator = (BaseObject& o)
{
	mesh = o.mesh;
	type = o.type;
	r = o.r;
	h = o.h;
	centery = o.centery;
	light = o.light;
	effect = o.effect;
	flags = o.flags;
	variants = o.variants;
	extraDist = o.extraDist;
	return *this;
}

//=================================================================================================
BaseObject* BaseObject::TryGet(int hash, ObjectGroup** outGroup)
{
	// find object
	BaseObject* obj = ContentItem<BaseObject>::TryGet(hash);
	if(obj)
		return obj;

	// find object group
	ObjectGroup* group = ObjectGroup::TryGet(hash);
	if(group)
	{
		if(outGroup)
			*outGroup = group;
		return group->GetRandom();
	}

	return nullptr;
}
