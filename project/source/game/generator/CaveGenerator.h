#pragma once

#include "MapGenerator.h"

//-----------------------------------------------------------------------------
struct CaveGenerator : MapGenerator
{
	~CaveGenerator();
	void Generate(Pole*& mapa, int size, Int2& stairs, int& stairs_dir, vector<Int2>& holes, Rect* ext, bool devmode);
	void RegenerateFlags(Pole* mapa, int size);

private:
	int GenerateInternal(int size);
	void FillMap(bool* m);
	void Make(bool* m1, bool* m2);
	int Finish(bool* m, bool* m2);
	int CalculateFill(bool* m, bool* m2, int start);
	int FillCave(bool* m, bool* m2, int start);

	int size = 45, size2 = size * size;
	bool* m1 = nullptr, *m2 = nullptr;
	int fill = 50;
	int iter = 3;
	int minx, miny, maxx, maxy;
	vector<int> v;
};
