#include "Pch.h"
#include "GameCore.h"
#include "BaseUsable.h"

//-----------------------------------------------------------------------------
vector<BaseUsable*> BaseUsable::usables;

//=================================================================================================
BaseUsable* BaseUsable::TryGet(Cstring id)
{
	for(auto use : usables)
	{
		if(use->id == id)
			return use;
	}

	return nullptr;
}
