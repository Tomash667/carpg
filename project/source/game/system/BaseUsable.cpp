#include "Pch.h"
#include "Core.h"
#include "BaseUsable.h"

vector<BaseUsable*> BaseUsable::usables;

BaseUsable* BaseUsable::TryGet(cstring id)
{
	assert(id);

	for(BaseUsable* use : usables)
	{
		if(use->id == id)
			return use;
	}

	return nullptr;
}
