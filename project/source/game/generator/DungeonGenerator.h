#pragma once

#include "LocationGenerator.h"

class DungeonGenerator : public LocationGenerator
{
public:
	int GetNumberOfSteps() override;
};
