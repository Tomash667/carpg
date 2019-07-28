#include "Pch.h"
#include "GameCore.h"
#include "Building.h"
#include "ContentLoader.h"
#include "Crc.h"
#include "UnitData.h"

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
