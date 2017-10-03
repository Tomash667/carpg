#include "Pch.h"
#include "Core.h"
#include "BaseUsable.h"

//-----------------------------------------------------------------------------
vector<BaseUsable*> BaseUsable::usables;

//=================================================================================================
BaseUsable* BaseUsable::TryGet(const AnyString& id)
{
	for(auto use : usables)
	{
		if(use->id == id)
			return use;
	}

	return nullptr;
}
