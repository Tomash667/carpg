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
	enum Mode
	{
		MODE_NORMAL,
		MODE_END_POINT,
		MODE_SMART
	};

	bool FindPath(LocationPart& locPart, const Int2& startTile, const Int2& targetTile, vector<Int2>& path, bool canOpenDoors = true, bool wandering = false, vector<Int2>* blocked = nullptr);
	int FindLocalPath(LocationPart& locPart, vector<Int2>& path, const Int2& myTile, Int2 targetTile, const Unit* me, const Unit* other, const void* usable = nullptr, Mode mode = MODE_NORMAL);
	void Draw(BasicShader* shader);
	void SetTarget(Unit* target) { marked = target; }
	bool IsDebugDraw() const { return marked != nullptr; }

private:
	vector<APoint> aMap;
	vector<bool> localPfMap;
	vector<pair<Vec2, int>> testPf;
	Unit* marked;
	bool testPfOutside;
};
