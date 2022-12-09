#pragma once

//-----------------------------------------------------------------------------
#include "InsideLocationGenerator.h"

//-----------------------------------------------------------------------------
class CaveGenerator final : public InsideLocationGenerator
{
public:
	CaveGenerator();
	~CaveGenerator();
	void Generate() override;
	void RegenerateFlags();
	void DebugDraw();
	void GenerateObjects() override;
	void GenerateUnits() override;
	void GenerateItems() override;
	int HandleUpdate(int days) override;

private:
	void FillMap(bool* m);
	void Make(bool* m1, bool* m2);
	int CalculateFill(bool* m, bool* m2, int start);
	int FillCave(bool* m, bool* m2, int start);
	int Finish(bool* m, bool* m2);
	int TryGenerate();
	void GenerateCave(Tile*& tiles, int size, Int2& stairs, GameDirection& stairsDir, vector<Int2>& holes, Rect* ext);
	void CreateStairs(Tile* tiles, Int2& stairs, GameDirection& stairsDir);
	void CreateHoles(Tile* tiles, vector<Int2>& holes);
	void GenerateCaveItems(int daysSince = 10);

	bool* m1, *m2;
	vector<int> v;
	int size, size2;
	int minx, miny, maxx, maxy;
	const int fill = 50;
	const int iter = 3;
	vector<Int2> sta;
};
