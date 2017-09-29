#include "Pch.h"
#include "Core.h"
#include "BaseObject.h"

vector<BaseObject*> BaseObject::objs;

//=================================================================================================
BaseObject* BaseObject::TryGet(cstring id, bool* is_variant)
{
	assert(id);

	if(strcmp(id, "painting") == 0)
	{
		if(is_variant)
			*is_variant = true;
		return TryGet(GetRandomPainting());
	}

	if(strcmp(id, "tombstone") == 0)
	{
		if(is_variant)
			*is_variant = true;
		int id = Random(0, 9);
		if(id != 0)
			return TryGet(Format("tombstone_x%d", id));
		else
			return TryGet("tombstone_1");
	}

	if(strcmp(id, "random") == 0)
	{
		switch(Rand() % 3)
		{
		case 0: return TryGet("wheel");
		case 1: return TryGet("rope");
		case 2: return TryGet("woodpile");
		}
	}

	for(BaseObject* obj : objs)
	{
		if(obj->id2 == id)
			return obj;
	}
	
	return nullptr;
}

//=================================================================================================
cstring BaseObject::GetRandomPainting()
{
	if(Rand() % 100 == 0)
		return "painting3";
	switch(Rand() % 23)
	{
	case 0:
		return "painting1";
	case 1:
	case 2:
		return "painting2";
	case 3:
	case 4:
		return "painting4";
	case 5:
	case 6:
		return "painting5";
	case 7:
	case 8:
		return "painting6";
	case 9:
		return "painting7";
	case 10:
		return "painting8";
	case 11:
	case 12:
	case 13:
		return "painting_x1";
	case 14:
	case 15:
	case 16:
		return "painting_x2";
	case 17:
	case 18:
	case 19:
		return "painting_x3";
	case 20:
	case 21:
	case 22:
	default:
		return "painting_x4";
	}
}
