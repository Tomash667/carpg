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
	void SetRoadSize(int road_size, int road_part);
	void GenerateMainRoad(RoadType type, GameDirection dir, int roads, bool plaza, int swap, vector<EntryPoint>& entry_points, int& gates, bool fill_roads);
	void GenerateBuildings(vector<ToBuild>& tobuild);
	void FlattenRoad();
	void SmoothTerrain();
	void CleanBorders();
	void FlattenRoadExits();
	void GenerateFields();
	void ApplyWallTiles(int gates);
	void GenerateRoads(TERRAIN_TILE road_tile, int tries);
	int MakeRoad(const Int2& pt, GameDirection dir, int road_index, int& collided_road);
	void FillRoad(const Int2& pt, GameDirection dir, int dist);
	bool MakeAndFillRoad(const Int2& pt, GameDirection dir, int road_index);
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
	void CreateRoadLineLeftRight(TERRAIN_TILE t, vector<EntryPoint>& entry_points);
	void CreateRoadLineBottomTop(TERRAIN_TILE t, vector<EntryPoint>& entry_points);
	void CreateRoadPartLeft(TERRAIN_TILE t, vector<EntryPoint>& entry_points);
	void CreateRoadPartRight(TERRAIN_TILE t, vector<EntryPoint>& entry_points);
	void CreateRoadPartBottom(TERRAIN_TILE t, vector<EntryPoint>& entry_points);
	void CreateRoadPartTop(TERRAIN_TILE t, vector<EntryPoint>& entry_points);
	void CreateRoadCenter(TERRAIN_TILE t);
	void CreateRoad(const Rect& r, TERRAIN_TILE t);
	void CreateCurveRoad(Int2 points[], uint count, TERRAIN_TILE t);
	void GeneratePath(const Int2& pt);
	void CreateEntry(vector<EntryPoint>& entry_points, EntryDir dir);
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

	City* city;
	TerrainTile* tiles;
	int w, h, road_part, road_size;
	float* height;
	vector<Pixel> pixels;
	int rs1, rs2;
	vector<APoint2> grid;
	APoint2Sorter2 sorter;
	vector<int> to_check;
	vector<Int2> tmp_pts;
	vector<Road> roads;
	vector<int> road_ids;
	TERRAIN_TILE road_tile;
	vector<pair<Int2, GameDirection>> valid_pts;
	vector<Rect> fields;
	Int2 well_pt;
	bool have_well;
};
