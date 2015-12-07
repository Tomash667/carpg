#pragma once

#include "GameCommon.h"
#include "TerrainTile.h"
#include "Building.h"
#include "Perlin.h"
#include "EntryPoint.h"

enum RoadType
{
	RoadType_Plus,
	RoadType_Line,
	RoadType_Curve,
	RoadType_Oval,
	RoadType_Three,
	RoadType_Sin,
	RoadType_Part,
	RoadType_Max
};

struct APoint2Sorter2
{
	explicit APoint2Sorter2(vector<APoint2>& grid) : grid(grid)
	{
	}

	bool operator() (int idx1, int idx2) const
	{
		return grid[idx1].koszt > grid[idx2].koszt;
	}

	vector<APoint2>& grid;
};

enum EntryDir
{
	ED_Left,
	ED_Right,
	ED_Bottom,
	ED_Top,
	ED_LeftBottom,
	ED_LeftTop,
	ED_RightBottom,
	ED_RightTop,
	ED_BottomLeft,
	ED_BottomRight,
	ED_TopLeft,
	ED_TopRight
};

#define ROAD_HORIZONTAL (1<<0)
#define ROAD_START_CHECKED (1<<1)
#define ROAD_END_CHECKED (1<<2)
#define ROAD_MID_CHECKED (1<<3)
#define ROAD_ALL_CHECKED (ROAD_START_CHECKED|ROAD_END_CHECKED|ROAD_MID_CHECKED)

struct Road2
{
	INT2 start, end;
	int flags;

	inline int Length() const
	{
		return max(abs(end.x - start.x), abs(end.y - start.y));
	}
};

class CityGenerator
{
public:
	CityGenerator() : sorter(grid)
	{

	}

	void Init(TerrainTile* tiles, float* height, int w, int h);
	void SetRoadSize(int road_size, int road_part);
	void SetTerrainNoise(int octaves, float frequency, float hmin, float hmax);
	void GenerateMainRoad(RoadType type, GAME_DIR dir, int roads, bool plaza, int swap, vector<EntryPoint>& entry_points, int& gates, bool fill_roads);
	void GenerateBuildings(vector<ToBuild>& tobuild);
	void RandomizeHeight();
	void FlattenRoad();
	void SmoothTerrain();
	void CleanBorders();
	void FlattenRoadExits();
	void GenerateFields();
	void ApplyWallTiles(int gates);
	void GenerateRoads(TERRAIN_TILE road_tile, int tries);
	int MakeRoad(const INT2& pt, GAME_DIR dir, int road_index, int& collided_road);
	void FillRoad(const INT2& pt, GAME_DIR dir, int dist);
	bool MakeAndFillRoad(const INT2& pt, GAME_DIR dir, int road_index);
	void CheckTiles(TERRAIN_TILE t);
	void Test();

	inline void AddRoad(const INT2& start, const INT2& end, int flags)
	{
		Road2& r = Add1(roads);
		r.start = start;
		r.end = end;
		r.flags = flags;
	}

private:
	void CreateRoadLineLeftRight(TERRAIN_TILE t, vector<EntryPoint>& entry_points);
	void CreateRoadLineBottomTop(TERRAIN_TILE t, vector<EntryPoint>& entry_points);
	void CreateRoadPartLeft(TERRAIN_TILE t, vector<EntryPoint>& entry_points);
	void CreateRoadPartRight(TERRAIN_TILE t, vector<EntryPoint>& entry_points);
	void CreateRoadPartBottom(TERRAIN_TILE t, vector<EntryPoint>& entry_points);
	void CreateRoadPartTop(TERRAIN_TILE t, vector<EntryPoint>& entry_points);
	void CreateRoadCenter(TERRAIN_TILE t);
	void CreateRoad(const Rect& r, TERRAIN_TILE t);
	void CreateCurveRoad(INT2 points[], uint count, TERRAIN_TILE t);
	void GeneratePath(const INT2& pt);
	void CreateEntry(vector<EntryPoint>& entry_points, EntryDir dir);

	TerrainTile* tiles;
	int w, h, road_part, road_size;
	float* height, hmin, hmax;
	vector<Pixel> pixels;
	int rs1, rs2;
	vector<APoint2> grid;
	APoint2Sorter2 sorter;
	vector<int> to_check;
	vector<INT2> tmp_pts;
	Perlin perlin;
	vector<Road2> roads;
	vector<int> road_ids;
	TERRAIN_TILE road_tile;
	vector<BuildPt> valid_pts;
};
