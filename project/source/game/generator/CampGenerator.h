#pragma once

#include "OutsideLocationGenerator.h"

class CampGenerator : public OutsideLocationGenerator
{
public:
	void Generate() override;
	int HandleUpdate() override;
	void GenerateObjects() override;
	void GenerateUnits() override;
	void GenerateItems() override;
};
