#include "Pch.h"
#include <unordered_set>
#include "GameCore.h"
#include "BaseObject.h"

//-----------------------------------------------------------------------------
BaseObject BaseObject::obj_alpha;
SetContainer<BaseObject> BaseObject::objs;
SetContainer<ObjectGroup> ObjectGroup::groups;

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
BaseObject* BaseObject::TryGet(Cstring id, ObjectGroup** out_group)
{
	// find object
	static BaseObject obj;
	obj.id = id;
	auto it = objs.find(&obj);
	if(it != objs.end())
		return *it;

	// find object group
	static ObjectGroup group;
	group.id = id;
	auto group_it = ObjectGroup::groups.find(&group);
	if(group_it != ObjectGroup::groups.end())
	{
		if(out_group)
			*out_group = (*group_it);
		BaseObject* obj = (*group_it)->GetRandom();
		return obj;
	}

	return nullptr;
}
