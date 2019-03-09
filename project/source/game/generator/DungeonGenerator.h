#pragma once

//-----------------------------------------------------------------------------
#include "InsideLocationGenerator.h"
#include "DungeonMapGenerator.h"

//-----------------------------------------------------------------------------
class DungeonGenerator final : public InsideLocationGenerator
{
public:
	void Generate() override;
	void GenerateObjects() override;
	void GenerateUnits() override;
	void GenerateItems() override;
	int HandleUpdate(int days) override;

private:
	void CreatePortal(InsideLocationLevel& lvl);
	void GenerateDungeonItems();

	DungeonMapGenerator map_gen;
};
