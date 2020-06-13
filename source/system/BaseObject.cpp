#include "Pch.h"
#include "BaseObject.h"

//-----------------------------------------------------------------------------
std::unordered_map<int, BaseObject*> ContentItem<BaseObject>::items;
std::unordered_map<int, ObjectGroup*> ContentItem<ObjectGroup>::items;

//=================================================================================================
BaseObject* ObjectGroup::EntryList::GetRandom()
{
	EntryList* list = this;
	while(true)
	{
		uint c = Rand() % list->total_chance;
		uint total = 0u;
		for(auto& e : list->entries)
		{
			total += e.chance;
			if(c <= total)
			{
				if(e.is_list)
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
		delete next_obj;
	else if(IsSet(flags, OBJ_MULTI_PHYSICS))
		delete[] next_obj;
}

//=================================================================================================
BaseObject& BaseObject::operator = (BaseObject& o)
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
BaseObject* BaseObject::TryGet(int hash, ObjectGroup** out_group)
{
	// find object
	BaseObject* obj = ContentItem<BaseObject>::TryGet(hash);
	if(obj)
		return obj;

	// find object group
	ObjectGroup* group = ObjectGroup::TryGet(hash);
	if(group)
	{
		if(out_group)
			*out_group = group;
		return group->GetRandom();
	}

	return nullptr;
}
