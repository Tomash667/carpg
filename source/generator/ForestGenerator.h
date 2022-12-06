#pragma once

//-----------------------------------------------------------------------------
#include "OutsideLocationGenerator.h"

//-----------------------------------------------------------------------------
class ForestGenerator final : public OutsideLocationGenerator
{
public:
	void Generate() override;
	int HandleUpdate(int days) override;
	void GenerateObjects() override;
	void GenerateUnits() override;
	void GenerateItems() override;
	void RandomizeTerrainTexture() override;

private:
	bool haveSawmill;
};
