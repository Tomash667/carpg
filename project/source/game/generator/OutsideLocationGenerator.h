#pragma once

#include "LocationGenerator.h"

class OutsideLocationGenerator : public LocationGenerator
{
public:
	void SpawnForestObjects(int road_dir = -1); //-1 brak, 0 -, 1 |
	void SpawnForestItems(int count_mod);
};
