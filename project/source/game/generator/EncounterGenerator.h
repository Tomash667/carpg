#pragma once

#include "LocationGenerator.h"

class EncounterGenerator : public LocationGenerator
{
public:
	int GetNumberOfSteps() override;
	void Generate() override;
};
