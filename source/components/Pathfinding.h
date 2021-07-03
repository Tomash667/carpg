#pragma once

//-----------------------------------------------------------------------------
class Pathfinding
{
	friend struct AStarSort;

	struct APoint
	{
		int dist, cost, sum, state;
		Int2 prev;

		bool IsLower(int sum2) const
		{
			return state == 0 || sum2 < sum;
		}
	};

public:
	bool FindPath(LocationPart& locPart, const Int2& start_tile, const Int2& target_tile, vector<Int2>& path, bool can_open_doors = true, bool wandering = false, vector<Int2>* blocked = nullptr);
	int FindLocalPath(LocationPart& locPart, vector<Int2>& path, const Int2& my_tile, const Int2& target_tile, const Unit* me, const Unit* other, const void* usable = nullptr, bool is_end_point = false);
	void Draw(BasicShader* shader);
	void SetTarget(Unit* target) { marked = target; }
	bool IsDebugDraw() const { return marked != nullptr; }

private:
	vector<APoint> a_map;
	vector<bool> local_pfmap;
	vector<pair<Vec2, int>> test_pf;
	Unit* marked;
	bool test_pf_outside;
};
