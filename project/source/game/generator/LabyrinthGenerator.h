#pragma once

#include "MapGenerator.h"

//-----------------------------------------------------------------------------
struct LabyrinthGenerator : MapGenerator
{
	void Generate(Pole*& mapa, const Int2& size, const Int2& room_size, Int2& stairs, int& stairs_dir, Int2& room_pos, int kratki_szansa, bool devmode);
private:
	int GenerateInternal(const Int2& maze_size, const Int2& room_size, Int2& room_pos, Int2& doors);
	inline bool IsInside(const Int2& pt, const Int2& start, const Int2& size)
	{
		return (pt.x >= start.x && pt.y >= start.y && pt.x < start.x + size.x && pt.y < start.y + size.y);
	}

	vector<bool> maze;
};
