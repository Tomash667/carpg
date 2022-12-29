#include "Pch.h"
#include "Building.h"

//-----------------------------------------------------------------------------
vector<Building*> Building::buildings;
std::map<string, Building*> Building::aliases;

//=================================================================================================
Building* Building::TryGet(Cstring id)
{
	for(Building* building : buildings)
	{
		if(building->id == id)
			return building;
	}

	auto it = aliases.find(string(id));
	if(it != aliases.end())
		return it->second;

	return nullptr;
}
