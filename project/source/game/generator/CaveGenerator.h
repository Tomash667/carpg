#pragma once

#include "LocationGenerator.h"

class CaveGenerator : public LocationGenerator
{
public:
	CaveGenerator();
	~CaveGenerator();
	int GetNumberOfSteps() override;
	void Generate() override;

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
	void DebugDraw();

	bool* m1, *m2;
	vector<int> v;
	int size, size2;
	int minx, miny, maxx, maxy;
	const int fill = 50;
	const int iter = 3;
};
