#pragma once

#include "OutsideLocationGenerator.h"

class MoonwellGenerator : public OutsideLocationGenerator
{
public:
	void Generate() override;
	void GenerateObjects() override;
	void GenerateUnits() override;
	void GenerateItems() override;
};
