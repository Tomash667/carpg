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
	bool HandleUpdate(int days) override;

private:
	void FillMap(bool* m);
	void Make(bool* m1, bool* m2);
	int CalculateFill(bool* m, bool* m2, int start);
	int FillCave(bool* m, bool* m2, int start);
	int Finish(bool* m, bool* m2);
	int TryGenerate();
	void GenerateCave(Pole*& tiles, int size, Int2& stairs, int& stairs_dir, vector<Int2>& holes, Rect* ext);
	void CreateStairs(Pole* tiles, Int2& stairs, int& stairs_dir);
	void CreateHoles(Pole* tiles, vector<Int2>& holes);
	void GenerateMushrooms(int days_since = 10);

	bool* m1, *m2;
	vector<int> v;
	int size, size2;
	int minx, miny, maxx, maxy;
	const int fill = 50;
	const int iter = 3;
};
