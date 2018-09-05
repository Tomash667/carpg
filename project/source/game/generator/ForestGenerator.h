#pragma once

#include "LocationGenerator.h"

class ForestGenerator : public LocationGenerator
{
public:
	int GetNumberOfSteps() override;
};
