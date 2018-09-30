#pragma once

//-----------------------------------------------------------------------------
#include "InsideLocationGenerator.h"

//-----------------------------------------------------------------------------
class DungeonGenerator final : public InsideLocationGenerator
{
public:
	void Generate() override;
	void GenerateObjects() override;
	void GenerateUnits() override;
	void GenerateItems() override;
	bool HandleUpdate(int days) override;

private:
	void GenerateDungeonItems();
};
