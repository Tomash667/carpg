#include "Pch.h"
#include "Building.h"

//-----------------------------------------------------------------------------
vector<Building*> Building::buildings;

//=================================================================================================
Building* Building::TryGet(Cstring id)
{
	for(auto building : buildings)
	{
		if(building->id == id)
			return building;
	}

	return nullptr;
}
