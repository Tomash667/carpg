#pragma once

//-----------------------------------------------------------------------------
#include "OutsideLocationGenerator.h"
#include "GameCommon.h"
#include "TerrainTile.h"
#include "Building.h"
#include "EntryPoint.h"

//-----------------------------------------------------------------------------
class CityGenerator final : public OutsideLocationGenerator
{
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

	struct BuildPt
	{
		Int2 pt;
		int side; // 0 = obojêtnie, 1 = <==> szerszy, 2 ^ d³u¿szy
	};

	struct APoint2
	{
		int cost, state, dir;
		Int2 prev;
	};

	struct APoint2Sorter
	{
		APoint2Sorter(APoint2* grid, uint s) : grid(grid), s(s)
		{
		}

		bool operator() (int idx1, int idx2) const
		{
			return grid[idx1].cost > grid[idx2].cost;
		}

		APoint2* grid;
		uint s;
	};

	struct APoint2Sorter2
	{
		explicit APoint2Sorter2(vector<APoint2>& grid) : grid(grid)
		{
		}

		bool operator() (int idx1, int idx2) const
		{
			return grid[idx1].cost > grid[idx2].cost;
		}

		vector<APoint2>& grid;
	};

	struct Road
	{
		Int2 start, end;
		int flags;

		int Length() const
		{
			return max(abs(end.x - start.x), abs(end.y - start.y));
		}
	};
public:
	CityGenerator() : sorter(grid)
	{
	}

	void Init() override;
	void Init(TerrainTile* tiles, float* height, int w, int h);
	void SetRoadSize(int roadSize, int roadPart);
	void GenerateMainRoad(RoadType type, GameDirection dir, int roads, bool plaza, int swap, vector<EntryPoint>& entryPoints, int& gates, bool fillRoads);
	void GenerateBuildings(vector<ToBuild>& tobuild);
	void FlattenRoad();
	void SmoothTerrain();
	void CleanBorders();
	void FlattenRoadExits();
	void GenerateFields();
	void ApplyWallTiles(int gates);
	void GenerateRoads(TERRAIN_TILE roadTile, int tries);
	int MakeRoad(const Int2& pt, GameDirection dir, int roadIndex, int& collidedRoad);
	void FillRoad(const Int2& pt, GameDirection dir, int dist);
	bool MakeAndFillRoad(const Int2& pt, GameDirection dir, int roadIndex);
	void CheckTiles(TERRAIN_TILE t);
	void Test();

	void AddRoad(const Int2& start, const Int2& end, int flags)
	{
		Road& r = Add1(roads);
		r.start = start;
		r.end = end;
		r.flags = flags;
	}

	int GetNumberOfSteps() override;
	void Generate() override;
	void OnEnter() override;

private:
	void CreateRoadLineLeftRight(TERRAIN_TILE t, vector<EntryPoint>& entryPoints);
	void CreateRoadLineBottomTop(TERRAIN_TILE t, vector<EntryPoint>& entryPoints);
	void CreateRoadPartLeft(TERRAIN_TILE t, vector<EntryPoint>& entryPoints);
	void CreateRoadPartRight(TERRAIN_TILE t, vector<EntryPoint>& entryPoints);
	void CreateRoadPartBottom(TERRAIN_TILE t, vector<EntryPoint>& entryPoints);
	void CreateRoadPartTop(TERRAIN_TILE t, vector<EntryPoint>& entryPoints);
	void CreateRoadCenter(TERRAIN_TILE t);
	void CreateRoad(const Rect& r, TERRAIN_TILE t);
	void CreateCurveRoad(Int2 points[], uint count, TERRAIN_TILE t);
	void GeneratePath(const Int2& pt);
	void CreateEntry(vector<EntryPoint>& entryPoints, EntryDir dir);
	bool IsPointNearRoad(int x, int y);
	void SpawnObjects();
	void SpawnBuildings();
	void SpawnUnits();
	void SpawnTemporaryUnits();
	void RemoveTemporaryUnits();
	void RepositionUnits();
	void GeneratePickableItems();
	void CreateMinimap() override;
	void OnLoad() override;
	void RespawnBuildingPhysics();
	void SetBuildingsParams();

	City* city;
	TerrainTile* tiles;
	int w, h, roadPart, roadSize;
	float* height;
	vector<Pixel> pixels;
	int rs1, rs2;
	vector<APoint2> grid;
	APoint2Sorter2 sorter;
	vector<int> toCheck;
	vector<Int2> tmpPts;
	vector<Road> roads;
	vector<int> roadIds;
	TERRAIN_TILE roadTile;
	vector<pair<Int2, GameDirection>> validPts;
	vector<Rect> fields;
	Int2 wellPt;
	bool haveWell;
};
