#pragma once

#include "InsideLocationGenerator.h"

class DungeonGenerator final : public InsideLocationGenerator
{
public:
	int GetNumberOfSteps() override;
	void Generate() override;
	void GenerateObjects() override;
	void GenerateUnits() override;
	void GenerateItems() override;
	bool HandleUpdate(int days) override;

private:
	void GenerateDungeonItems();
};
