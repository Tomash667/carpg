#pragma once

//-----------------------------------------------------------------------------
#include "GameComponent.h"

//-----------------------------------------------------------------------------
class Pathfinding : public GameComponent
{
	friend struct AStarSort;

	struct APoint
	{
		int odleglosc, koszt, suma, stan;
		Int2 prev;

		bool IsLower(int suma2) const
		{
			return stan == 0 || suma2 < suma;
		}
	};

public:
	void Cleanup() override { delete this; }
	bool FindPath(LevelContext& ctx, const Int2& start_tile, const Int2& target_tile, vector<Int2>& path, bool can_open_doors = true, bool wedrowanie = false, vector<Int2>* blocked = nullptr);
	int FindLocalPath(LevelContext& ctx, vector<Int2>& path, const Int2& my_tile, const Int2& target_tile, const Unit* me, const Unit* other, const void* usable = nullptr, bool is_end_point = false);
	void Draw(DebugDrawer* dd);
	void SetDebugDraw(bool enabled) { debug_draw = enabled; }
	bool IsDebugDraw() const { return debug_draw; }

private:
	vector<APoint> a_map;
	vector<bool> local_pfmap;
	vector<std::pair<Vec2, int>> test_pf;
	Unit* marked;
	bool test_pf_outside, debug_draw;
};
