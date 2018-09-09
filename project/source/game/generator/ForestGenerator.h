#pragma once

#include "OutsideLocationGenerator.h"

class ForestGenerator final : public OutsideLocationGenerator
{
public:
	void Generate() override;
	int HandleUpdate() override;
	void GenerateObjects() override;
	void GenerateUnits() override;
	void GenerateItems() override;

private:
	bool have_sawmill;
};
