#pragma once

#include "TypeItem.h"

//-----------------------------------------------------------------------------
struct Building;

//-----------------------------------------------------------------------------
// Building group
struct BuildingGroup : public TypeItem
{
	string id;
	vector<Building*> buildings;
};
