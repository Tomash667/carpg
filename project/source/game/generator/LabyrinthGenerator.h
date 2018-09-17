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
	void GenerateLabyrinth(Pole*& tiles, const Int2& size, const Int2& room_size, Int2& stairs, int& stairs_dir, Int2& room_pos, int grating_chance);
	void CreateStairs(Pole* tiles, const Int2& size, Int2& stairs, int& stairs_dir);
	void CreateGratings(Pole* tiles, const Int2& size, const Int2& room_size, const Int2& room_pos, int grating_chance);

	vector<bool> maze;
};
