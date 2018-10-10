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
	int TryGenerate(const Int2& maze_size, const Int2& room_size, Int2& room_pos, Int2& doors);
	void GenerateLabyrinth(Tile*& tiles, const Int2& size, const Int2& room_size, Int2& stairs, GameDirection& stairs_dir, Int2& room_pos, int grating_chance);
	void CreateStairs(Tile* tiles, const Int2& size, Int2& stairs, GameDirection& stairs_dir);
	void CreateGratings(Tile* tiles, const Int2& size, const Int2& room_size, const Int2& room_pos, int grating_chance);

	vector<bool> maze;
};
