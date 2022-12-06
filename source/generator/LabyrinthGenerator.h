#pragma once

//-----------------------------------------------------------------------------
#include "InsideLocationGenerator.h"

//-----------------------------------------------------------------------------
class LabyrinthGenerator final : public InsideLocationGenerator
{
public:
	void Generate() override;
	void GenerateObjects() override;
	void GenerateUnits() override;

private:
	int TryGenerate(const Int2& mazeSize, const Int2& roomSize, Int2& roomPos, Int2& doors);
	void GenerateLabyrinth(Tile*& tiles, const Int2& size, const Int2& roomSize, Int2& stairs, GameDirection& stairsDir, Int2& roomPos, int gratingChance);
	void CreateStairs(Tile* tiles, const Int2& size, Int2& stairs, GameDirection& stairsDir);
	void CreateGratings(Tile* tiles, const Int2& size, const Int2& roomSize, const Int2& roomPos, int gratingChance);

	vector<bool> maze;
};
