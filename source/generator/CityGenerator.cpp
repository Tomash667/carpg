#include "Pch.h"
#include "GameCore.h"
#include "CityGenerator.h"
#include "Location.h"
#include "City.h"
#include "World.h"
#include "Level.h"
#include "Terrain.h"
#include "QuestManager.h"
#include "Quest.h"
#include "Quest_Contest.h"
#include "Team.h"
#include "OutsideObject.h"
#include "AIController.h"
#include "Texture.h"
#include "Arena.h"
#include "Game.h"
#include "Object.h"

enum RoadFlags
{
	ROAD_HORIZONTAL = 1 << 0,
	ROAD_START_CHECKED = 1 << 1,
	ROAD_END_CHECKED = 1 << 2,
	ROAD_MID_CHECKED = 1 << 3,
	ROAD_ALL_CHECKED = ROAD_START_CHECKED | ROAD_END_CHECKED | ROAD_MID_CHECKED
};

const float SPAWN_RATIO = 0.2f;
const float SPAWN_RANGE = 4.f;
const float EXIT_WIDTH = 1.3f;
const float EXIT_START = 11.1f;
const float EXIT_END = 12.6f;

//=================================================================================================
void CityGenerator::Init()
{
	OutsideLocationGenerator::Init();
	city = static_cast<City*>(outside);
}

//=================================================================================================
void CityGenerator::Init(TerrainTile* _tiles, float* _height, int _w, int _h)
{
	assert(_tiles && _height && _w > 0 && _h > 0);
	tiles = _tiles;
	height = _height;
	w = _w;
	h = _h;

	roads.clear();
	road_ids.clear();
	road_ids.resize(w*h, -1);
}

//=================================================================================================
void CityGenerator::SetRoadSize(int _road_size, int _road_part)
{
	assert(_road_size > 0 && _road_part >= 0);
	road_size = _road_size;
	rs1 = road_size / 2;
	rs2 = road_size / 2;
	road_part = _road_part;
}

//=================================================================================================
void CityGenerator::GenerateMainRoad(RoadType type, GameDirection dir, int rocky_roads, bool plaza, int swap, vector<EntryPoint>& entry_points,
	int& gates, bool fill_roads)
{
	memset(tiles, 0, sizeof(TerrainTile)*w*h);

#define W1 (w/2-rs1)
#define W2 (w/2+rs2)
#define H1 (h/2-rs1)
#define H2 (h/2+rs2)

	switch(type)
	{
	// dir oznacza którêdy idzie g³ówna droga, swap która droga jest druga (swap=0 lewa,dó³; swap=1 prawa,góra) jeœli nie s¹ tego samego typu pod³o¿a
	case RoadType_Plus:
		gates = GATE_NORTH | GATE_SOUTH | GATE_EAST | GATE_WEST;
		if(rocky_roads >= 3 || rocky_roads == 0)
		{
			// plus
			TERRAIN_TILE t = (rocky_roads != 0 ? TT_ROAD : TT_SAND);
			CreateRoadLineLeftRight(t, entry_points);
			CreateRoadLineBottomTop(t, entry_points);
		}
		else if(dir == GDIR_LEFT || dir == GDIR_RIGHT)
		{
			CreateRoadLineLeftRight(TT_ROAD, entry_points);
			if(rocky_roads > 1)
			{
				if(!swap)
				{
					CreateRoadPartBottom(TT_ROAD, entry_points);
					CreateRoadPartTop(TT_SAND, entry_points);
				}
				else
				{
					CreateRoadPartBottom(TT_SAND, entry_points);
					CreateRoadPartTop(TT_ROAD, entry_points);
				}
			}
			else
			{
				CreateRoadPartBottom(TT_SAND, entry_points);
				CreateRoadPartTop(TT_SAND, entry_points);
			}
		}
		else
		{
			CreateRoadLineBottomTop(TT_ROAD, entry_points);
			if(rocky_roads > 1)
			{
				if(!swap)
				{
					CreateRoadPartLeft(TT_ROAD, entry_points);
					CreateRoadPartRight(TT_SAND, entry_points);
				}
				else
				{
					CreateRoadPartLeft(TT_SAND, entry_points);
					CreateRoadPartRight(TT_ROAD, entry_points);
				}
			}
			else
			{
				CreateRoadPartLeft(TT_SAND, entry_points);
				CreateRoadPartRight(TT_SAND, entry_points);
			}
		}
		if(fill_roads)
		{
			AddRoad(Int2(0, h / 2), Int2(w / 2, h / 2), ROAD_HORIZONTAL | ROAD_START_CHECKED | ROAD_END_CHECKED);
			AddRoad(Int2(w / 2, h / 2), Int2(w, h / 2), ROAD_HORIZONTAL | ROAD_START_CHECKED | ROAD_END_CHECKED);
			AddRoad(Int2(w / 2, 0), Int2(w / 2, h / 2), ROAD_START_CHECKED | ROAD_END_CHECKED);
			AddRoad(Int2(w / 2, h / 2), Int2(w / 2, h), ROAD_START_CHECKED | ROAD_END_CHECKED);
		}
		break;

	case RoadType_Line:
		if(dir == GDIR_LEFT || dir == GDIR_RIGHT)
		{
			gates = GATE_EAST | GATE_WEST;
			if(fill_roads)
			{
				AddRoad(Int2(0, h / 2), Int2(w / 2, h / 2), ROAD_HORIZONTAL | ROAD_START_CHECKED | ROAD_END_CHECKED);
				AddRoad(Int2(w / 2, h / 2), Int2(w, h / 2), ROAD_HORIZONTAL | ROAD_START_CHECKED | ROAD_END_CHECKED);
			}
		}
		else
		{
			gates = GATE_NORTH | GATE_SOUTH;
			if(fill_roads)
			{
				AddRoad(Int2(w / 2, 0), Int2(w / 2, h / 2), ROAD_START_CHECKED | ROAD_END_CHECKED);
				AddRoad(Int2(w / 2, h / 2), Int2(w / 2, h), ROAD_START_CHECKED | ROAD_END_CHECKED);
			}
		}

		if(rocky_roads >= 2 || rocky_roads == 0)
		{
			TERRAIN_TILE t = (rocky_roads != 0 ? TT_ROAD : TT_SAND);
			if(dir == GDIR_LEFT || dir == GDIR_RIGHT)
				CreateRoadLineLeftRight(t, entry_points);
			else
				CreateRoadLineBottomTop(t, entry_points);
		}
		else
		{
			CreateRoadCenter(TT_ROAD);
			switch(dir)
			{
			case GDIR_LEFT:
				CreateRoadPartLeft(TT_ROAD, entry_points);
				CreateRoadPartRight(TT_SAND, entry_points);
				break;
			case GDIR_RIGHT:
				CreateRoadPartRight(TT_ROAD, entry_points);
				CreateRoadPartLeft(TT_SAND, entry_points);
				break;
			case GDIR_DOWN:
				CreateRoadPartBottom(TT_ROAD, entry_points);
				CreateRoadPartTop(TT_SAND, entry_points);
				break;
			case GDIR_UP:
				CreateRoadPartTop(TT_ROAD, entry_points);
				CreateRoadPartBottom(TT_SAND, entry_points);
				break;
			}
		}
		break;

	case RoadType_Curve:
		{
			gates = 0;

			Int2 pts[3];
			pts[1] = Int2(w / 2, h / 2);
			TERRAIN_TILE t = (rocky_roads > 0 ? TT_ROAD : TT_SAND);

			switch(dir)
			{
			case GDIR_LEFT:
				CreateRoad(Rect(0, H1, road_part, H2), t);
				CreateEntry(entry_points, ED_Left);
				pts[0] = Int2(road_part, h / 2);
				if(!swap)
				{
					CreateRoad(Rect(W1, 0, W2, road_part), t);
					CreateEntry(entry_points, ED_Bottom);
					pts[2] = Int2(w / 2, road_part);
				}
				else
				{
					CreateRoad(Rect(W1, h - road_part, W2, h - 1), t);
					CreateEntry(entry_points, ED_Top);
					pts[2] = Int2(w / 2, h - road_part);
				}
				break;
			case GDIR_RIGHT:
				CreateRoad(Rect(w - road_part, H1, w - 1, H2), t);
				CreateEntry(entry_points, ED_Right);
				pts[0] = Int2(w - road_part, h / 2);
				if(!swap)
				{
					CreateRoad(Rect(W1, 0, W2, road_part), t);
					CreateEntry(entry_points, ED_Bottom);
					pts[2] = Int2(w / 2, road_part);
				}
				else
				{
					CreateRoad(Rect(W1, h - road_part, W2, h - 1), t);
					CreateEntry(entry_points, ED_Top);
					pts[2] = Int2(w / 2, h - road_part);
				}
				break;
			case GDIR_DOWN:
				CreateRoad(Rect(W1, 0, W2, road_part), t);
				CreateEntry(entry_points, ED_Bottom);
				pts[0] = Int2(w / 2, road_part);
				if(!swap)
				{
					CreateRoad(Rect(0, H1, road_part, H2), t);
					CreateEntry(entry_points, ED_Left);
					pts[2] = Int2(road_part, h / 2);
				}
				else
				{
					CreateRoad(Rect(w - road_part, H1, w - 1, H2), t);
					CreateEntry(entry_points, ED_Right);
					pts[2] = Int2(w - road_part, h / 2);
				}
				break;
			case GDIR_UP:
				CreateRoad(Rect(W1, h - road_part, W2, h - 1), t);
				CreateEntry(entry_points, ED_Top);
				pts[0] = Int2(w / 2, h - road_part);
				if(!swap)
				{
					CreateRoad(Rect(0, H1, road_part, H2), t);
					CreateEntry(entry_points, ED_Left);
					pts[2] = Int2(road_part, h / 2);
				}
				else
				{
					CreateRoad(Rect(w - road_part, H1, w - 1, H2), t);
					CreateEntry(entry_points, ED_Right);
					pts[2] = Int2(w - road_part, h / 2);
				}
				break;
			}

			CreateCurveRoad(pts, 3, t);
		}
		break;

	case RoadType_Oval:
		{
			TERRAIN_TILE t = (rocky_roads != 0 ? TT_ROAD : TT_SAND);
			if(dir == GDIR_LEFT || dir == GDIR_RIGHT)
			{
				gates = GATE_WEST | GATE_EAST;
				CreateRoad(Rect(0, H1, road_part, H2), t);
				CreateEntry(entry_points, ED_Left);
				CreateRoad(Rect(w - road_part, H1, w - 1, H2), t);
				CreateEntry(entry_points, ED_Right);
				Int2 pts[3];
				pts[0] = Int2(road_part, h / 2);
				pts[1] = Int2(w / 2, road_part);
				pts[2] = Int2(w - road_part, h / 2);
				CreateCurveRoad(pts, 3, t);
				pts[1] = Int2(w / 2, h - road_part);
				CreateCurveRoad(pts, 3, t);
			}
			else
			{
				gates = GATE_NORTH | GATE_SOUTH;
				CreateRoad(Rect(W1, 0, W2, road_part), t);
				CreateEntry(entry_points, ED_Bottom);
				CreateRoad(Rect(W1, h - road_part, W2, h - 1), t);
				CreateEntry(entry_points, ED_Top);
				Int2 pts[3];
				pts[0] = Int2(w / 2, road_part);
				pts[1] = Int2(road_part, h / 2);
				pts[2] = Int2(w / 2, h - road_part);
				CreateCurveRoad(pts, 3, t);
				pts[1] = Int2(w - road_part, h / 2);
				CreateCurveRoad(pts, 3, t);
			}
		}
		break;

		// dir oznacza brakuj¹c¹ drogê, swap kolejnoœæ (0-5)
	case RoadType_Three:
		if(rocky_roads >= 3 || rocky_roads == 0)
		{
			TERRAIN_TILE t = (rocky_roads != 0 ? TT_ROAD : TT_SAND);
			switch(dir)
			{
			case GDIR_LEFT:
				gates = GATE_NORTH | GATE_SOUTH | GATE_EAST;
				CreateRoadLineBottomTop(t, entry_points);
				CreateRoadPartRight(t, entry_points);
				break;
			case GDIR_RIGHT:
				gates = GATE_NORTH | GATE_SOUTH | GATE_WEST;
				CreateRoadLineBottomTop(t, entry_points);
				CreateRoadPartLeft(t, entry_points);
				break;
			case GDIR_DOWN:
				gates = GATE_NORTH | GATE_EAST | GATE_WEST;
				CreateRoadLineLeftRight(t, entry_points);
				CreateRoadPartTop(t, entry_points);
				break;
			case GDIR_UP:
				gates = GATE_SOUTH | GATE_EAST | GATE_WEST;
				CreateRoadLineLeftRight(t, entry_points);
				CreateRoadPartBottom(t, entry_points);
				break;
			}
		}
		else
		{
			static const int mod[6][3] = {
				{2, 1, 0},
				{2, 0, 1},
				{1, 2, 0},
				{0, 2, 1},
				{1, 0, 2},
				{0, 1, 2}
			};
#define GetMod(x) t[mod[swap][x]]
			CreateRoadCenter(TT_ROAD);
			TERRAIN_TILE t[3] = { TT_ROAD, rocky_roads > 1 ? TT_ROAD : TT_SAND, TT_SAND };
			switch(dir)
			{
			case GDIR_LEFT:
				gates = GATE_NORTH | GATE_SOUTH | GATE_EAST;
				CreateRoadPartRight(GetMod(0), entry_points);
				CreateRoadPartBottom(GetMod(1), entry_points);
				CreateRoadPartTop(GetMod(2), entry_points);
				break;
			case GDIR_RIGHT:
				gates = GATE_NORTH | GATE_SOUTH | GATE_WEST;
				CreateRoadPartLeft(GetMod(0), entry_points);
				CreateRoadPartBottom(GetMod(1), entry_points);
				CreateRoadPartTop(GetMod(2), entry_points);
				break;
			case GDIR_DOWN:
				gates = GATE_NORTH | GATE_EAST | GATE_WEST;
				CreateRoadPartTop(GetMod(0), entry_points);
				CreateRoadPartLeft(GetMod(1), entry_points);
				CreateRoadPartRight(GetMod(2), entry_points);
				break;
			case GDIR_UP:
				gates = GATE_SOUTH | GATE_EAST | GATE_WEST;
				CreateRoadPartBottom(GetMod(0), entry_points);
				CreateRoadPartLeft(GetMod(1), entry_points);
				CreateRoadPartRight(GetMod(2), entry_points);
				break;
			}
#undef GetMod
		}
		if(fill_roads)
		{
			if(dir != GDIR_LEFT)
				AddRoad(Int2(0, h / 2), Int2(w / 2, h / 2), ROAD_HORIZONTAL | ROAD_START_CHECKED | ROAD_END_CHECKED);
			if(dir != GDIR_RIGHT)
				AddRoad(Int2(w / 2, h / 2), Int2(w, h / 2), ROAD_HORIZONTAL | ROAD_START_CHECKED | ROAD_END_CHECKED);
			if(dir != GDIR_DOWN)
				AddRoad(Int2(w / 2, 0), Int2(w / 2, h / 2), ROAD_START_CHECKED | ROAD_END_CHECKED);
			if(dir != GDIR_UP)
				AddRoad(Int2(w / 2, h / 2), Int2(w / 2, h), ROAD_START_CHECKED | ROAD_END_CHECKED);
		}
		break;

	case RoadType_Sin:
		{
			gates = 0;

			TERRAIN_TILE t = (rocky_roads != 0 ? TT_ROAD : TT_SAND);
			Int2 pts[4];
			switch(dir)
			{
			case GDIR_LEFT:
				CreateRoad(Rect(0, h / 2 - road_part - rs1, road_part, h / 2 - road_part + rs2), t);
				CreateEntry(entry_points, ED_LeftBottom);
				CreateRoad(Rect(w - road_part, h / 2 + road_part - rs1, w - 1, h / 2 + road_part + rs2), t);
				CreateEntry(entry_points, ED_RightTop);
				pts[0] = Int2(road_part, h / 2 - road_part);
				pts[1] = Int2(w / 2, h / 2 - road_part);
				pts[2] = Int2(w / 2, h / 2 + road_part);
				pts[3] = Int2(w - road_part, h / 2 + road_part);
				break;
			case GDIR_RIGHT:
				CreateRoad(Rect(w - road_part, h / 2 - road_part - rs1, w - 1, h / 2 - road_part + rs2), t);
				CreateEntry(entry_points, ED_RightBottom);
				CreateRoad(Rect(0, h / 2 + road_part - rs1, road_part, h / 2 + road_part + rs2), t);
				CreateEntry(entry_points, ED_LeftTop);
				pts[0] = Int2(w - road_part, h / 2 - road_part);
				pts[2] = Int2(w / 2, h / 2 + road_part);
				pts[1] = Int2(w / 2, h / 2 - road_part);
				pts[3] = Int2(road_part, h / 2 + road_part);
				break;
			case GDIR_DOWN:
				CreateRoad(Rect(w / 2 - road_part - rs1, 0, w / 2 - road_part + rs2, road_part), t);
				CreateEntry(entry_points, ED_BottomLeft);
				CreateRoad(Rect(w / 2 + road_part - rs1, h - road_part, w / 2 + road_part + rs2, h - 1), t);
				CreateEntry(entry_points, ED_TopRight);
				pts[0] = Int2(w / 2 - road_part, road_part);
				pts[1] = Int2(w / 2 - road_part, h / 2);
				pts[2] = Int2(w / 2 + road_part, h / 2);
				pts[3] = Int2(w / 2 + road_part, h - road_part);
				break;
			case GDIR_UP:
				CreateRoad(Rect(w / 2 - road_part - rs1, h - road_part, w / 2 - road_part + rs2, h - 1), t);
				CreateEntry(entry_points, ED_TopLeft);
				CreateRoad(Rect(w / 2 + road_part - rs1, 0, w / 2 + road_part + rs2, road_part), t);
				CreateEntry(entry_points, ED_BottomRight);
				pts[0] = Int2(w / 2 - road_part, h - road_part);
				pts[1] = Int2(w / 2 - road_part, h / 2);
				pts[2] = Int2(w / 2 + road_part, h / 2);
				pts[3] = Int2(w / 2 + road_part, road_part);
				break;
			}
			CreateCurveRoad(pts, 4, t);
		}
		break;

	case RoadType_Part:
		{
			TERRAIN_TILE t = (rocky_roads != 0 ? TT_ROAD : TT_SAND);
			CreateRoadCenter(t);
			switch(dir)
			{
			case GDIR_LEFT:
				gates = GATE_WEST;
				CreateRoadPartLeft(t, entry_points);
				break;
			case GDIR_RIGHT:
				gates = GATE_EAST;
				CreateRoadPartRight(t, entry_points);
				break;
			case GDIR_DOWN:
				gates = GATE_SOUTH;
				CreateRoadPartBottom(t, entry_points);
				break;
			case GDIR_UP:
				gates = GATE_NORTH;
				CreateRoadPartTop(t, entry_points);
				break;
			}
		}
		break;

	default:
		assert(0);
		break;
	}

	if(plaza)
	{
		Int2 center(w / 2, h / 2);
		for(int y = -5; y <= 5; ++y)
		{
			for(int x = -5; x <= 5; ++x)
			{
				Int2 pt = Int2(center.x - x, center.y - y);
				if(Vec3::Distance(PtToPos(pt), PtToPos(center)) <= 10.f)
					tiles[pt(w)].Set(rocky_roads > 0 ? TT_ROAD : TT_SAND, TT_GRASS, 0, TM_ROAD);
			}
		}
	}

	if(!roads.empty())
	{
		for(int i = 0; i < (int)roads.size(); ++i)
		{
			const Road2& r = roads[i];
			int minx = max(0, r.start.x - 1),
				miny = max(0, r.start.y - 1),
				maxx = min(w - 1, r.end.x + 1),
				maxy = min(h - 1, r.end.y + 1);
			for(int y = miny; y <= maxy; ++y)
			{
				for(int x = minx; x <= maxx; ++x)
					road_ids[x + y * w] = i;
			}
		}
	}
}

//=================================================================================================
void CityGenerator::CreateRoad(const Rect& r, TERRAIN_TILE t)
{
	assert(r.p1.x >= 0 && r.p1.y >= 0 && r.p2.x < w && r.p2.y < h);
	for(int y = r.p1.y; y <= r.p2.y; ++y)
	{
		for(int x = r.p1.x; x <= r.p2.x; ++x)
		{
			TerrainTile& tt = tiles[x + y * w];
			tt.t = t;
			tt.t2 = t;
			tt.alpha = 0;
			tt.mode = TM_ROAD;
		}
	}
}

//=================================================================================================
// nie obs³uguje krawêdzi!
void CityGenerator::CreateCurveRoad(Int2 points[], uint count, TERRAIN_TILE t)
{
	assert(count == 3 || count == 4);

	if(count == 3)
		Pixel::PlotQuadBezier(points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y, 1.f, (float)road_size, pixels);
	else
		Pixel::PlotCubicBezier(points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y, points[3].x, points[3].y, (float)road_size, pixels);

	// zrób coœ z pixelami
	for(vector<Pixel>::iterator it = pixels.begin(), end = pixels.end(); it != end; ++it)
	{
		TerrainTile& tile = tiles[it->pt(w)];
		if(tile.t != t)
		{
			tile.t2 = tile.t;
			tile.t = t;
			tile.mode = TM_ROAD;
			tile.alpha = it->alpha;
		}
		// gdy to by³o to pojawia³ siê piach zamiast drogi :S
		else if(tile.alpha > 0)
			tile.alpha = byte((float(tile.alpha) / 255 * float(it->alpha) / 255) * 255);
	}
	pixels.clear();
}

//=================================================================================================
void CityGenerator::GenerateBuildings(vector<ToBuild>& tobuild)
{
	// budynki
	for(vector<ToBuild>::iterator build_it = tobuild.begin(), build_end = tobuild.end(); build_it != build_end; ++build_it)
	{
		Int2 ext = build_it->building->size - Int2(1, 1);

		bool ok;
		vector<BuildPt> points;
		int side;

		// side - w któr¹ stronê mo¿e byæ obrócony budynek w tym miejscu
		// -1 - brak
		// 0 - po x
		// 1 - po z
		// 2 - po x i po z

		// miejsca na domy
		const int ymin = int(0.25f*w);
		const int ymax = int(0.75f*w);
		for(int y = ymin; y < ymax; ++y)
		{
			for(int x = ymin; x < ymax; ++x)
			{
#define CHECK_TILE(_x,_y) ok = true; \
	for(int _yy=-1; _yy<=1 && ok; ++_yy) { \
	for(int _xx=-1; _xx<=1; ++_xx) { \
	if(tiles[(_x)+_xx+((_y)+_yy)*w].t != TT_GRASS) { ok = false; break; } } }

				CHECK_TILE(x, y);
				if(!ok)
					continue;

				if(ext.x == ext.y)
				{
					int a = ext.x / 2;
					int b = ext.x - a;

					for(int yy = -a; yy <= b; ++yy)
					{
						for(int xx = -a; xx <= b; ++xx)
						{
							CHECK_TILE(x + xx, y + yy);
							if(!ok)
								goto superbreak;
						}
					}
				superbreak:

					if(ok)
						side = 2;
					else
						side = -1;
				}
				else
				{
					int a1 = ext.x / 2;
					int b1 = ext.x - a1;
					int a2 = ext.y / 2;
					int b2 = ext.y - a2;

					side = -1;

					// po x
					for(int yy = -a1; yy <= b1; ++yy)
					{
						for(int xx = -a2; xx <= b2; ++xx)
						{
							CHECK_TILE(x + xx, y + yy);
							if(!ok)
								goto superbreak2;
						}
					}
				superbreak2:

					if(ok)
						side = 0;

					// po z
					for(int yy = -a2; yy <= b2; ++yy)
					{
						for(int xx = -a1; xx <= b1; ++xx)
						{
							CHECK_TILE(x + xx, y + yy);
							if(!ok)
								goto superbreak3;
						}
					}
				superbreak3:

					if(ok)
					{
						if(side == 0)
							side = 2;
						else
							side = 1;
					}
				}

				if(side != -1)
				{
					BuildPt& bpt = Add1(points);
					bpt.pt = Int2(x, y);
					bpt.side = side;
				}
			}
		}

		if(points.empty())
		{
			if(!build_it->required)
			{
				tobuild.erase(build_it, tobuild.end());
				break;
			}
			Error("Failed to generate city map! No place for building %s!", build_it->building->id.c_str());
			return;
		}

		// ustal pozycjê i obrót budynku
		const Int2 centrum(w / 2, w / 2);
		int range, best_range = INT_MAX, index = 0;
		for(vector<BuildPt>::iterator it = points.begin(), end = points.end(); it != end; ++it, ++index)
		{
			int best_length = 999;
			GameDirection dir = GDIR_INVALID;

			// calculate distance to closest road
			if(it->side == 1 || it->side == 2)
			{
				// down
				int length = 1;
				Int2 pt = it->pt;
				--pt.y;

				while(1)
				{
					TerrainTile& t = tiles[pt.x + pt.y*w];
					if(t.mode == TM_ROAD)
					{
						if(t.t == TT_SAND)
							length = length * 2 + 5;
						break;
					}
					--pt.y;
					if(pt.y < ymin)
					{
						length = 1000;
						break;
					}
					++length;
				}

				if(tiles[pt.x + pt.y*w].mode == TM_ROAD && length < best_length)
				{
					best_length = length;
					dir = GDIR_DOWN;
				}

				// up
				length = 1;
				pt = it->pt;
				++pt.y;

				while(1)
				{
					TerrainTile& t = tiles[pt.x + pt.y*w];
					if(t.mode == TM_ROAD)
					{
						if(t.t == TT_SAND)
							length = length * 2 + 5;
						break;
					}
					++pt.y;
					if(pt.y > ymax)
					{
						length = 1000;
						break;
					}
					++length;
				}

				if(tiles[pt.x + pt.y*w].mode == TM_ROAD && length < best_length)
				{
					best_length = length;
					dir = GDIR_UP;
				}
			}
			if(it->side == 0 || it->side == 2)
			{
				// left
				int length = 1;
				Int2 pt = it->pt;
				--pt.x;

				while(1)
				{
					TerrainTile& t = tiles[pt.x + pt.y*w];
					if(t.mode == TM_ROAD)
					{
						if(t.t == TT_SAND)
							length = length * 2 + 5;
						break;
					}
					--pt.x;
					if(pt.x < ymin)
					{
						length = 1000;
						break;
					}
					++length;
				}

				if(tiles[pt.x + pt.y*w].mode == TM_ROAD && length < best_length)
				{
					best_length = length;
					dir = GDIR_LEFT;
				}

				// right
				length = 1;
				pt = it->pt;
				++pt.x;

				while(1)
				{
					TerrainTile& t = tiles[pt.x + pt.y*w];
					if(t.mode == TM_ROAD)
					{
						if(t.t == TT_SAND)
							length = length * 2 + 5;
						break;
					}
					++pt.x;
					if(pt.x > ymax)
					{
						length = 1000;
						break;
					}
					++length;
				}

				if(tiles[pt.x + pt.y*w].mode == TM_ROAD && length < best_length)
				{
					best_length = length;
					dir = GDIR_RIGHT;
				}
			}

			if(dir == GDIR_INVALID)
			{
				if(it->side == 2)
					dir = (GameDirection)(Rand() % 4);
				else if(it->side == 1)
				{
					if(Rand() % 2 == 0)
						dir = GDIR_DOWN;
					else
						dir = GDIR_UP;
				}
				else
				{
					if(Rand() % 2 == 0)
						dir = GDIR_LEFT;
					else
						dir = GDIR_RIGHT;
				}
			}

			if(IsSet(build_it->building->flags, Building::FAVOR_CENTER))
				range = Int2::Distance(centrum, it->pt);
			else
				range = 0;
			if(IsSet(build_it->building->flags, Building::FAVOR_ROAD))
				range += max(0, best_length - 1);
			else
				range += max(0, best_length - 5);

			if(range <= best_range)
			{
				if(range < best_range)
				{
					valid_pts.clear();
					best_range = range;
				}
				valid_pts.push_back(std::make_pair(it->pt, dir));
			}
		}

		pair<Int2, GameDirection> pt = RandomItem(valid_pts);
		GameDirection best_dir = pt.second;
		valid_pts.clear();

		// 0 - obrócony w góre
		// w:4 h:2
		// 8765
		// 4321
		// kolejnoœæ po x i y odwrócona

		// 1 - obrócony w lewo
		// 51
		// 62
		// 73
		// 84
		// x = y, x odwrócone

		// 2 - obrócony w dó³
		// w:4 h:2
		// 1234
		// 5678
		// domyœlnie, nic nie zmieniaj

		// 3 - obrócony w prawo
		// 48
		// 37
		// 26
		// 15
		// x = y, x i y odwrócone

		build_it->pt = pt.first;
		build_it->rot = best_dir;

		Int2 ext2 = build_it->building->size;
		if(best_dir == GDIR_LEFT || best_dir == GDIR_RIGHT)
			std::swap(ext2.x, ext2.y);

		const int x1 = (ext2.x - 1) / 2;
		const int x2 = ext2.x - x1 - 1;
		const int y1 = (ext2.y - 1) / 2;
		const int y2 = ext2.y - y1 - 1;

		Int2 road_start(-1, -1);
		int count = 0;
		float sum = 0;

		for(int yy = -y1, yr = 0; yy <= y2; ++yy, ++yr)
		{
			for(int xx = -x1, xr = 0; xx <= x2; ++xx, ++xr)
			{
				Building::TileScheme scheme;
				switch(best_dir)
				{
				case GDIR_DOWN:
					scheme = build_it->building->scheme[xr + (ext2.y - yr - 1)*ext2.x];
					break;
				case GDIR_LEFT:
					scheme = build_it->building->scheme[ext2.y - yr - 1 + (ext2.x - xr - 1)*ext2.y];
					break;
				case GDIR_UP:
					scheme = build_it->building->scheme[ext2.x - xr - 1 + yr * ext2.x];
					break;
				case GDIR_RIGHT:
					scheme = build_it->building->scheme[yr + xr * ext2.y];
					break;
				default:
					assert(0);
					break;
				}

				Int2 pt2(pt.first.x + xx, pt.first.y + yy);

				TerrainTile& t = tiles[pt2.x + (pt2.y)*w];
				assert(t.t == TT_GRASS);

				switch(scheme)
				{
				case Building::SCHEME_GRASS:
					break;
				case Building::SCHEME_BUILDING:
					t.Set(TT_SAND, TM_BUILDING_BLOCK);
					break;
				case Building::SCHEME_SAND:
					t.Set(TT_SAND, TM_BUILDING_SAND);
					break;
				case Building::SCHEME_PATH:
					assert(road_start == Int2(-1, -1));
					road_start = pt2;
					break;
				case Building::SCHEME_UNIT:
					t.Set(TT_SAND, TM_BUILDING_SAND);
					build_it->unit_pt = pt2;
					break;
				case Building::SCHEME_BUILDING_PART:
					t.Set(TT_SAND, TM_BUILDING);
					break;
				}
			}
		}

		for(int yy = -y1, yr = 0; yy <= y2 + 1; ++yy, ++yr)
		{
			for(int xx = -x1, xr = 0; xx <= x2 + 1; ++xx, ++xr)
			{
				Int2 pt2(pt.first.x + xx, pt.first.y + yy);
				++count;
				sum += height[pt2.x + pt2.y*(w + 1)];
				tmp_pts.push_back(pt2);
			}
		}

		// set height
		sum /= count;
		for(Int2& pt : tmp_pts)
			height[pt.x + pt.y*(w + 1)] = sum;
		tmp_pts.clear();

		assert(road_start != Int2(-1, -1));

		if(road_start != Int2(-1, -1))
			GeneratePath(road_start);
	}
}

//=================================================================================================
void CityGenerator::GeneratePath(const Int2& pt)
{
	assert(pt.x >= 0 && pt.y >= 0 && pt.x < w && pt.y < h);

	int size = w * h;
	if(size > (int)grid.size())
		grid.resize(size);
	memset(&grid[0], 0, sizeof(APoint2)*size);

	int start_idx = pt.x + pt.y*w;
	to_check.push_back(start_idx);

	grid[start_idx].stan = 1;
	grid[start_idx].dir = -1;

	struct Mod
	{
		Int2 change, back;
		int cost, cost2;

		Mod(const Int2& change, const Int2& back, int cost, int cost2) : change(change), back(back), cost(cost), cost2(cost2)
		{
		}
	};
	static const Mod mod[8] = {
		Mod(Int2(-1,0), Int2(1,0), 10, 15),
		Mod(Int2(1,0), Int2(-1,0), 10, 15),
		Mod(Int2(0,-1), Int2(0,1), 10, 15),
		Mod(Int2(0,1), Int2(0,-1), 10, 15),
		Mod(Int2(-1,-1), Int2(1,1), 20, 30),
		Mod(Int2(-1,1), Int2(1,-1), 20, 30),
		Mod(Int2(1,-1), Int2(-1,1), 20, 30),
		Mod(Int2(1,1), Int2(-1,-1), 20, 30)
	};

	int end_tile_idx = -1;
	const int x_min = int(float(w)*0.2f);
	const int x_max = int(float(w)*0.8f);
	const int y_min = int(float(h)*0.2f);
	const int y_max = int(float(h)*0.8f);

	while(!to_check.empty())
	{
		int pt_idx = to_check.back();
		Int2 pt(pt_idx%w, pt_idx / w);
		to_check.pop_back();
		if(pt.x <= x_min || pt.x >= x_max || pt.y <= y_min || pt.y >= y_max)
			continue;

		APoint2& this_point = grid[pt_idx];

		for(int i = 0; i < 8; ++i)
		{
			const int idx = pt_idx + mod[i].change.x + mod[i].change.y*w;
			APoint2& point = grid[idx];
			if(point.stan == 0)
			{
				TerrainTile& tile = tiles[idx];
				if(tile.mode == TM_ROAD)
				{
					point.prev = pt;
					point.dir = i;
					end_tile_idx = idx;
					goto superbreak;
				}
				else if(tile.mode == TM_NORMAL || tile.mode == TM_BUILDING_SAND)
				{
					point.prev = pt;
					point.stan = 1;
					point.koszt = this_point.koszt + (this_point.dir == i ? mod[i].cost : mod[i].cost2);
					point.dir = i;
					to_check.push_back(idx);
				}
				else
					point.stan = 1;
			}
		}

		std::sort(to_check.begin(), to_check.end(), sorter);
	}

superbreak:
	to_check.clear();

	assert(end_tile_idx != -1);

	Int2 pt2(end_tile_idx % w, end_tile_idx / w);
	bool go = true;
	while(go)
	{
		TerrainTile& t = tiles[pt2.x + pt2.y*w];
		if(t.t == TT_GRASS)
			t.Set(TT_SAND, TM_PATH);
		else if(t.mode == TM_ROAD)
		{
			if(t.t == TT_ROAD)
			{
				if(t.alpha != 0)
				{
					t.t = TT_SAND;
					t.t2 = TT_ROAD;
					t.alpha = 255 - t.alpha;
				}
			}
			else
			{
				t.alpha = 0;
				t.t = TT_SAND;
			}
		}
		if(pt == pt2)
			go = false;
		const APoint2& apt = grid[pt2.x + pt2.y*w];
		if(apt.dir > 3)
		{
			const Mod& m = mod[apt.dir];
			{
				TerrainTile& t = tiles[pt2.x + m.back.x + pt2.y*w];
				if(t.t == TT_GRASS)
					t.Set(TT_SAND, TT_GRASS, 96, TM_PATH);
				else if(t.mode == TM_ROAD)
				{
					if(t.t == TT_ROAD)
						t.t2 = TT_SAND;
					else if(t.alpha > 96)
						t.alpha = (t.alpha + 96) / 2;
				}
			}
			{
				TerrainTile& t = tiles[pt2.x + (pt2.y + m.back.y)*w];
				if(t.t == TT_GRASS)
					t.Set(TT_SAND, TT_GRASS, 96, TM_PATH);
				else if(t.mode == TM_ROAD)
				{
					if(t.t == TT_ROAD)
						t.t2 = TT_SAND;
					else if(t.alpha > 96)
						t.alpha = (t.alpha + 96) / 2;
				}
			}
		}
		pt2 = apt.prev;
	}
}

// this terrain smoothing is a bit too complex
//    9 10 11
//     o--o     o - height points (left down from X is current point)
//20  6| 2| 7 12
//  o--o--o--o
//19| 1| 0| 3|13
//  o--o--o--o
//18  5| 4| 8 14
//     o--o
//   17 16 15
const Int2 blocked[] = {
	Int2(0,0),
	Int2(-1,0),
	Int2(0,1),
	Int2(1,0),
	Int2(0,-1),
	Int2(-1,-1),
	Int2(-1,1),
	Int2(1,1),
	Int2(1,-1),
	Int2(-1,2),
	Int2(0,2),
	Int2(1,2),
	Int2(2,1),
	Int2(2,0),
	Int2(2,-1),
	Int2(1,-2),
	Int2(0,-2),
	Int2(-1,-2),
	Int2(-2,-1),
	Int2(-2,0),
	Int2(-2,1)
};
struct Nei
{
	Int2 pt;
	int id[4];
};
const Nei neis[] = {
	Int2(0,0), 1, 5, 4, 0,
	Int2(0,1), 1, 6, 2, 0,
	Int2(1,1), 0, 2, 7, 3,
	Int2(1,0), 0, 3, 4, 8,
	Int2(-1,0), 18, 19, 1, 5,
	Int2(-1,1), 19, 20, 6, 1,
	Int2(0,2), 6, 9, 10, 2,
	Int2(1,2), 2, 10, 11, 7,
	Int2(2,1), 3, 7, 12, 13,
	Int2(2,0), 8, 3, 13, 14,
	Int2(1,-1), 16, 4, 8, 15,
	Int2(0,-1), 17, 5, 4, 16
};

//=================================================================================================
void CityGenerator::FlattenRoad()
{
	const int w1 = w + 1;
	bool block[21];
	block[0] = true;

	for(int y = 2; y < h - 2; ++y)
	{
		for(int x = 2; x < w - 2; ++x)
		{
			if(OR2_EQ(tiles[x + y * w].mode, TM_ROAD, TM_PATH))
			{
				for(int i = 1; i < 21; ++i)
					block[i] = !tiles[x + blocked[i].x + (y + blocked[i].y)*w].IsBuilding();

				float sum = 0.f;
				for(int i = 0; i < 12; ++i)
				{
					const Nei& nei = neis[i];
					Int2 pt(x + nei.pt.x, y + nei.pt.y);
					sum += height[pt(w1)];
					if(block[nei.id[0]] && block[nei.id[1]] && block[nei.id[2]] && block[nei.id[3]])
						tmp_pts.push_back(pt);
				}

				sum /= 12;
				for(Int2& pt : tmp_pts)
					height[pt(w1)] = sum;
				tmp_pts.clear();
			}
		}
	}
}

//=================================================================================================
void CityGenerator::SmoothTerrain()
{
	for(int y = 1; y < h; ++y)
	{
		for(int x = 1; x < w; ++x)
		{
			if(tiles[x + y * w].mode < TM_BUILDING_SAND && tiles[x - 1 + y * w].mode < TM_BUILDING_SAND && tiles[x + (y - 1)*w].mode < TM_BUILDING_SAND && tiles[x - 1 + (y - 1)*w].mode < TM_BUILDING_SAND)
			{
				float sum = (height[x + y * (w + 1)] + height[x - 1 + y * (w + 1)] + height[x + 1 + y * (w + 1)] + height[x + (y - 1)*(h + 1)] + height[x + (y + 1)*(h + 1)]) / 5;
				height[x + y * (w + 1)] = sum;
			}
		}
	}
}

//=================================================================================================
void CityGenerator::CreateRoadLineLeftRight(TERRAIN_TILE t, vector<EntryPoint>& entry_points)
{
	CreateRoad(Rect(0, h / 2 - rs1, w - 1, h / 2 + rs2), t);
	CreateEntry(entry_points, ED_Left);
	CreateEntry(entry_points, ED_Right);
}

//=================================================================================================
void CityGenerator::CreateRoadLineBottomTop(TERRAIN_TILE t, vector<EntryPoint>& entry_points)
{
	CreateRoad(Rect(w / 2 - rs1, 0, w / 2 + rs2, h - 1), t);
	CreateEntry(entry_points, ED_Bottom);
	CreateEntry(entry_points, ED_Top);
}

//=================================================================================================
void CityGenerator::CreateRoadPartLeft(TERRAIN_TILE t, vector<EntryPoint>& entry_points)
{
	CreateRoad(Rect(0, h / 2 - rs1, w / 2 - rs1 - 1, h / 2 + rs2), t);
	CreateEntry(entry_points, ED_Left);
}

//=================================================================================================
void CityGenerator::CreateRoadPartRight(TERRAIN_TILE t, vector<EntryPoint>& entry_points)
{
	CreateRoad(Rect(w / 2 + rs2 + 1, h / 2 - rs1, w - 1, h / 2 + rs2), t);
	CreateEntry(entry_points, ED_Right);
}

//=================================================================================================
void CityGenerator::CreateRoadPartBottom(TERRAIN_TILE t, vector<EntryPoint>& entry_points)
{
	CreateRoad(Rect(w / 2 - rs1, 0, w / 2 + rs1, h / 2 - rs1 - 1), t);
	CreateEntry(entry_points, ED_Bottom);
}

//=================================================================================================
void CityGenerator::CreateRoadPartTop(TERRAIN_TILE t, vector<EntryPoint>& entry_points)
{
	CreateRoad(Rect(w / 2 - rs1, h / 2 + rs2 + 1, w / 2 + rs2, h - 1), t);
	CreateEntry(entry_points, ED_Top);
}

//=================================================================================================
void CityGenerator::CreateRoadCenter(TERRAIN_TILE t)
{
	CreateRoad(Rect(w / 2 - rs1, h / 2 - rs1, w / 2 + rs2, h / 2 + rs2), t);
}

//=================================================================================================
void CityGenerator::CreateEntry(vector<EntryPoint>& entry_points, EntryDir dir)
{
	EntryPoint& ep = Add1(entry_points);

	switch(dir)
	{
	case ED_Left:
		{
			Vec2 p(SPAWN_RATIO*w * 2, float(h) + 1);
			ep.spawn_rot = PI * 3 / 2;
			ep.spawn_region = Box2d(p.x - SPAWN_RANGE, p.y - SPAWN_RANGE, p.x + SPAWN_RANGE, p.y + SPAWN_RANGE);
			ep.exit_region = Box2d(p.x - EXIT_END, p.y - EXIT_WIDTH, p.x - EXIT_START, p.y + EXIT_WIDTH);
		}
		break;
	case ED_Right:
		{
			Vec2 p((1.f - SPAWN_RATIO)*w * 2, float(h) + 1);
			ep.spawn_rot = PI / 2;
			ep.spawn_region = Box2d(p.x - SPAWN_RANGE, p.y - SPAWN_RANGE, p.x + SPAWN_RANGE, p.y + SPAWN_RANGE);
			ep.exit_region = Box2d(p.x + EXIT_START, p.y - EXIT_WIDTH, p.x + EXIT_END, p.y + EXIT_WIDTH);
		}
		break;
	case ED_Bottom:
		{
			Vec2 p(float(w) + 1, SPAWN_RATIO*h * 2);
			ep.spawn_rot = PI;
			ep.spawn_region = Box2d(p.x - SPAWN_RANGE, p.y - SPAWN_RANGE, p.x + SPAWN_RANGE, p.y + SPAWN_RANGE);
			ep.exit_region = Box2d(p.x - EXIT_WIDTH, p.y - EXIT_END, p.x + EXIT_WIDTH, p.y - EXIT_START);
		}
		break;
	case ED_Top:
		{
			Vec2 p(float(w) + 1, (1.f - SPAWN_RATIO)*h * 2);
			ep.spawn_rot = 0;
			ep.spawn_region = Box2d(p.x - SPAWN_RANGE, p.y - SPAWN_RANGE, p.x + SPAWN_RANGE, p.y + SPAWN_RANGE);
			ep.exit_region = Box2d(p.x - EXIT_WIDTH, p.y + EXIT_START, p.x + EXIT_WIDTH, p.y + EXIT_END);
		}
		break;
	case ED_LeftBottom:
		{
			Vec2 p(SPAWN_RATIO*w * 2, float(h / 2 - road_part) * 2 + 1);
			ep.spawn_rot = PI * 3 / 2;
			ep.spawn_region = Box2d(p.x - SPAWN_RANGE, p.y - SPAWN_RANGE, p.x + SPAWN_RANGE, p.y + SPAWN_RANGE);
			ep.exit_region = Box2d(p.x - EXIT_END, p.y - EXIT_WIDTH, p.x + EXIT_START, p.y + EXIT_WIDTH);
		}
		break;
	case ED_LeftTop:
		{
			Vec2 p(SPAWN_RATIO*w * 2, float(h / 2 + road_part) * 2 + 1);
			ep.spawn_rot = PI * 3 / 2;
			ep.spawn_region = Box2d(p.x - SPAWN_RANGE, p.y - SPAWN_RANGE, p.x + SPAWN_RANGE, p.y + SPAWN_RANGE);
			ep.exit_region = Box2d(p.x - EXIT_END, p.y - EXIT_WIDTH, p.x + EXIT_START, p.y + EXIT_WIDTH);
		}
		break;
	case ED_RightBottom:
		{
			Vec2 p((1.f - SPAWN_RATIO)*w * 2, float(h / 2 - road_part) * 2 + 1);
			ep.spawn_rot = PI / 2;
			ep.spawn_region = Box2d(p.x - SPAWN_RANGE, p.y - SPAWN_RANGE, p.x + SPAWN_RANGE, p.y + SPAWN_RANGE);
			ep.exit_region = Box2d(p.x + EXIT_START, p.y - EXIT_WIDTH, p.x + EXIT_END, p.y + EXIT_WIDTH);
		}
		break;
	case ED_RightTop:
		{
			Vec2 p((1.f - SPAWN_RATIO)*w * 2, float(h / 2 + road_part) * 2 + 1);
			ep.spawn_rot = PI / 2;
			ep.spawn_region = Box2d(p.x - SPAWN_RANGE, p.y - SPAWN_RANGE, p.x + SPAWN_RANGE, p.y + SPAWN_RANGE);
			ep.exit_region = Box2d(p.x + EXIT_START, p.y - EXIT_WIDTH, p.x + EXIT_END, p.y + EXIT_WIDTH);
		}
		break;
	case ED_BottomLeft:
		{
			Vec2 p(float(w / 2 - road_part) * 2 + 1, SPAWN_RATIO*h * 2);
			ep.spawn_rot = PI;
			ep.spawn_region = Box2d(p.x - SPAWN_RANGE, p.y - SPAWN_RANGE, p.x + SPAWN_RANGE, p.y + SPAWN_RANGE);
			ep.exit_region = Box2d(p.x - EXIT_WIDTH, p.y - EXIT_END, p.x + EXIT_WIDTH, p.y + EXIT_START);
		}
		break;
	case ED_BottomRight:
		{
			Vec2 p(float(w / 2 + road_part) * 2 + 1, SPAWN_RATIO*h * 2);
			ep.spawn_rot = PI;
			ep.spawn_region = Box2d(p.x - SPAWN_RANGE, p.y - SPAWN_RANGE, p.x + SPAWN_RANGE, p.y + SPAWN_RANGE);
			ep.exit_region = Box2d(p.x - EXIT_WIDTH, p.y - EXIT_END, p.x + EXIT_WIDTH, p.y + EXIT_START);
		}
		break;
	case ED_TopLeft:
		{
			Vec2 p(float(w / 2 - road_part) * 2 + 1, (1.f - SPAWN_RATIO)*h * 2);
			ep.spawn_rot = 0;
			ep.spawn_region = Box2d(p.x - SPAWN_RANGE, p.y - SPAWN_RANGE, p.x + SPAWN_RANGE, p.y + SPAWN_RANGE);
			ep.exit_region = Box2d(p.x - EXIT_WIDTH, p.y + EXIT_START, p.x + EXIT_WIDTH, p.y + EXIT_END);
		}
		break;
	case ED_TopRight:
		{
			Vec2 p(float(w / 2 + road_part) * 2 + 1, (1.f - SPAWN_RATIO)*h * 2);
			ep.spawn_rot = 0;
			ep.spawn_region = Box2d(p.x - SPAWN_RANGE, p.y - SPAWN_RANGE, p.x + SPAWN_RANGE, p.y + SPAWN_RANGE);
			ep.exit_region = Box2d(p.x - EXIT_WIDTH, p.y + EXIT_START, p.x + EXIT_WIDTH, p.y + EXIT_END);
		}
		break;
	}

	ep.exit_y = 0;
}

//=================================================================================================
void CityGenerator::CleanBorders()
{
	// bottom / top
	for(int x = 1; x < w; ++x)
	{
		height[x] = height[x + (w + 1)];
		height[x + h * (w + 1)] = height[x + (h - 1)*(w + 1)];
	}

	// left / right
	for(int y = 1; y < h; ++y)
	{
		height[y*(w + 1)] = height[y*(w + 1) + 1];
		height[(y + 1)*(w + 1) - 1] = height[(y + 1)*(w + 1) - 2];
	}

	// corners
	height[0] = (height[1] + height[w + 1]) / 2;
	height[w] = (height[w - 1] + height[(w + 1) * 2 - 1]) / 2;
	height[h*(w + 1)] = (height[1 + h * (w + 1)] + height[(h - 1)*(w + 1)]) / 2;
	height[(h + 1)*(w + 1) - 1] = (height[(h + 1)*(w + 1) - 2] + height[h*(w + 1) - 1]) / 2;
}

//=================================================================================================
void CityGenerator::FlattenRoadExits()
{
	// left
	for(int yy = 0; yy < h; ++yy)
	{
		if(tiles[15 + yy * w].mode == TM_ROAD)
		{
			float th = height[15 + yy * (w + 1)];
			for(int y = 0; y <= h; ++y)
			{
				for(int x = 0; x < 15; ++x)
				{
					if(IsPointNearRoad(x, y))
						height[x + y * (w + 1)] = th;
				}
			}
			break;
		}
	}

	// right
	for(int yy = 0; yy < h; ++yy)
	{
		if(tiles[w - 15 + yy * w].mode == TM_ROAD)
		{
			float th = height[w - 15 + yy * (w + 1)];
			for(int y = 0; y <= h; ++y)
			{
				for(int x = w - 15; x < w; ++x)
				{
					if(IsPointNearRoad(x, y))
						height[x + y * (w + 1)] = th;
				}
			}
			break;
		}
	}

	// bottom
	for(int xx = 0; xx < w; ++xx)
	{
		if(tiles[xx + 15 * w].mode == TM_ROAD)
		{
			float th = height[xx + 15 * (w + 1)];
			for(int y = 0; y < 15; ++y)
			{
				for(int x = 0; x <= w; ++x)
				{
					if(IsPointNearRoad(x, y))
						height[x + y * (w + 1)] = th;
				}
			}
			break;
		}
	}

	// top
	for(int xx = 0; xx < w; ++xx)
	{
		if(tiles[xx + (h - 15)*w].mode == TM_ROAD)
		{
			float th = height[xx + (h - 15)*(w + 1)];
			for(int y = h - 15; y <= h; ++y)
			{
				for(int x = 0; x <= w; ++x)
				{
					if(IsPointNearRoad(x, y))
						height[x + y * (w + 1)] = th;
				}
			}
			break;
		}
	}
}

//=================================================================================================
void CityGenerator::GenerateFields()
{
	const int ymin = int(0.25f*w);
	const int ymax = int(0.75f*w) - 5;

	for(int i = 0; i < 50; ++i)
	{
		Int2 pt(Random(ymin, ymax), Random(ymin, ymax));
		if(tiles[pt.x + pt.y*w].mode != TM_NORMAL)
			continue;

		int fw = Random(4, 8);
		int fh = Random(4, 8);
		if(fw > fh)
			fw *= 2;
		else
			fh *= 2;

		for(int y = pt.y - 1; y <= pt.y + fh; ++y)
		{
			for(int x = pt.x - 1; x <= pt.x + fw; ++x)
			{
				if(tiles[x + y * w].mode != TM_NORMAL)
					goto next;
			}
		}

		for(int y = pt.y; y < pt.y + fh; ++y)
		{
			for(int x = pt.x; x < pt.x + fw; ++x)
			{
				tiles[x + y * w].Set(TT_FIELD, TM_FIELD);
				float sum = (height[x + y * (w + 1)] + height[x + y * (w + 1)] + height[x + (y + 1)*(w + 1)] + height[x + 1 + (y - 1)*(w + 1)] + height[x + 1 + (y + 1)*(w + 1)]) / 5;
				height[x + y * (w + 1)] = sum;
				height[x + (y + 1)*(w + 1)] = sum;
				height[x + 1 + (y - 1)*(w + 1)] = sum;
				height[x + 1 + (y + 1)*(w + 1)] = sum;
			}
		}

	next:;
	}
}

//=================================================================================================
void CityGenerator::ApplyWallTiles(int gates)
{
	const int mur1 = int(0.15f*w);
	const int mur2 = int(0.85f*w);
	const int w1 = w + 1;

	// tiles under walls
	for(int i = mur1; i <= mur2; ++i)
	{
		// north
		tiles[i + mur1 * w].Set(TT_SAND, TM_BUILDING);
		if(tiles[i + (mur1 + 1)*w].t == TT_GRASS)
			tiles[i + (mur1 + 1)*w].Set(TT_SAND, TT_GRASS, 128, TM_BUILDING);
		height[i + (mur1 - 2) * w1] = 1.f;
		height[i + (mur1 - 1) * w1] = 1.f;
		height[i + mur1 * w1] = 1.f;
		height[i + (mur1 + 1)*w1] = 1.f;
		height[i + (mur1 + 2)*w1] = 1.f;
		// south
		tiles[i + mur2 * w].Set(TT_SAND, TM_BUILDING);
		if(tiles[i + (mur2 - 1)*w].t == TT_GRASS)
			tiles[i + (mur2 - 1)*w].Set(TT_SAND, TT_GRASS, 128, TM_BUILDING);
		height[i + (mur2 + 2)*w1] = 1.f;
		height[i + (mur2 + 1)*w1] = 1.f;
		height[i + mur2 * w1] = 1.f;
		height[i + (mur2 - 1)*w1] = 1.f;
		height[i + (mur2 - 2)*w1] = 1.f;
		// west
		tiles[mur1 + i * w].Set(TT_SAND, TM_BUILDING);
		if(tiles[mur1 + 1 + i * w].t == TT_GRASS)
			tiles[mur1 + 1 + i * w].Set(TT_SAND, TT_GRASS, 128, TM_BUILDING);
		height[mur1 - 2 + i * w1] = 1.f;
		height[mur1 - 1 + i * w1] = 1.f;
		height[mur1 + i * w1] = 1.f;
		height[mur1 + 1 + i * w1] = 1.f;
		height[mur1 + 2 + i * w1] = 1.f;
		// east
		tiles[mur2 + i * w].Set(TT_SAND, TM_BUILDING);
		if(tiles[mur2 - 1 + i * w].t == TT_GRASS)
			tiles[mur2 - 1 + i * w].Set(TT_SAND, TT_GRASS, 128, TM_BUILDING);
		height[mur2 + 2 + i * w1] = 1.f;
		height[mur2 + 1 + i * w1] = 1.f;
		height[mur2 + i * w1] = 1.f;
		height[mur2 - 1 + i * w1] = 1.f;
		height[mur2 - 2 + i * w1] = 1.f;
	}

	// tiles under gates
	if(IsSet(gates, GATE_SOUTH))
	{
		tiles[w / 2 - 1 + int(0.15f*w)*w].Set(TT_ROAD, TM_ROAD);
		tiles[w / 2 + int(0.15f*w)*w].Set(TT_ROAD, TM_ROAD);
		tiles[w / 2 + 1 + int(0.15f*w)*w].Set(TT_ROAD, TM_ROAD);
		tiles[w / 2 - 1 + (int(0.15f*w) + 1)*w].Set(TT_ROAD, TM_ROAD);
		tiles[w / 2 + (int(0.15f*w) + 1)*w].Set(TT_ROAD, TM_ROAD);
		tiles[w / 2 + 1 + (int(0.15f*w) + 1)*w].Set(TT_ROAD, TM_ROAD);
	}
	if(IsSet(gates, GATE_WEST))
	{
		tiles[int(0.15f*w) + (w / 2 - 1)*w].Set(TT_ROAD, TM_ROAD);
		tiles[int(0.15f*w) + (w / 2)*w].Set(TT_ROAD, TM_ROAD);
		tiles[int(0.15f*w) + (w / 2 + 1)*w].Set(TT_ROAD, TM_ROAD);
		tiles[int(0.15f*w) + 1 + (w / 2 - 1)*w].Set(TT_ROAD, TM_ROAD);
		tiles[int(0.15f*w) + 1 + (w / 2)*w].Set(TT_ROAD, TM_ROAD);
		tiles[int(0.15f*w) + 1 + (w / 2 + 1)*w].Set(TT_ROAD, TM_ROAD);
	}
	if(IsSet(gates, GATE_NORTH))
	{
		tiles[w / 2 - 1 + int(0.85f*w)*w].Set(TT_ROAD, TM_ROAD);
		tiles[w / 2 + int(0.85f*w)*w].Set(TT_ROAD, TM_ROAD);
		tiles[w / 2 + 1 + int(0.85f*w)*w].Set(TT_ROAD, TM_ROAD);
		tiles[w / 2 - 1 + (int(0.85f*w) - 1)*w].Set(TT_ROAD, TM_ROAD);
		tiles[w / 2 + (int(0.85f*w) - 1)*w].Set(TT_ROAD, TM_ROAD);
		tiles[w / 2 + 1 + (int(0.85f*w) - 1)*w].Set(TT_ROAD, TM_ROAD);
	}
	if(IsSet(gates, GATE_EAST))
	{
		tiles[int(0.85f*w) + (w / 2 - 1)*w].Set(TT_ROAD, TM_ROAD);
		tiles[int(0.85f*w) + (w / 2)*w].Set(TT_ROAD, TM_ROAD);
		tiles[int(0.85f*w) + (w / 2 + 1)*w].Set(TT_ROAD, TM_ROAD);
		tiles[int(0.85f*w) - 1 + (w / 2 - 1)*w].Set(TT_ROAD, TM_ROAD);
		tiles[int(0.85f*w) - 1 + (w / 2)*w].Set(TT_ROAD, TM_ROAD);
		tiles[int(0.85f*w) - 1 + (w / 2 + 1)*w].Set(TT_ROAD, TM_ROAD);
	}
}

/*
	----------------
	-A============B-
	----------------

	A start
	B end

	distance for ROAD_CHECK, ROAD_WIDTH is measured like this

	-----X----- width = 5
	   --X--    width = 2
*/

#define RT_START 0
#define RT_END 1
#define RT_MID 2

const int ROAD_MIN_DIST = 10;
const int ROAD_MIN_MID_SPLIT = 25;
const int ROAD_CHECK = 6;
const int ROAD_WIDTH = 1;
const int ROAD_DUAL_CHANCE = 4; // 100/x %
const int ROAD_DUAL_CHANCE_IF_FAIL = 2; // 100/x %
const int ROAD_JOIN_CHANCE = 4; // 100/x %
const int ROAD_TRIPLE_CHANCE = 10; // 100/x %
const float CITY_BORDER_MIN = 0.2f;
const float CITY_BORDER_MAX = 0.8f;

int get_choice_pop(int* choice, int& choices)
{
	int index = Rand() % choices;
	int value = choice[index];
	--choices;
	std::swap(choice[choices], choice[index]);
	return value;
}

//=================================================================================================
void CityGenerator::GenerateRoads(TERRAIN_TILE _road_tile, int tries)
{
	road_tile = _road_tile;
	to_check.clear();
	for(int i = 0; i < (int)roads.size(); ++i)
		to_check.push_back(i);

	const int ROAD_MIN_X = (int)(CITY_BORDER_MIN*w),
		ROAD_MAX_X = (int)(CITY_BORDER_MAX*w),
		ROAD_MIN_Y = (int)(CITY_BORDER_MIN*h),
		ROAD_MAX_Y = (int)(CITY_BORDER_MAX*h);
	int choice[3];

	for(int i = 0; i < tries; ++i)
	{
		if(to_check.empty())
			break;

		int index = RandomItemPop(to_check);
		Road2& r = roads[index];

		int choices = 0;
		if(!IsSet(r.flags, ROAD_START_CHECKED))
			choice[choices++] = RT_START;
		if(!IsSet(r.flags, ROAD_MID_CHECKED))
			choice[choices++] = RT_MID;
		if(!IsSet(r.flags, ROAD_END_CHECKED))
			choice[choices++] = RT_END;
		if(choices == 0)
			continue;

		const int type = choice[Rand() % choices];

		Int2 pt;
		const bool horizontal = IsSet(r.flags, ROAD_HORIZONTAL);

		if(type == RT_MID)
		{
			r.flags |= ROAD_MID_CHECKED;
			Int2 rstart = r.start, rend = r.end;
			if(rstart.x < ROAD_MIN_X)
				r.start.x = ROAD_MIN_X;
			if(rstart.y < ROAD_MIN_Y)
				r.start.y = ROAD_MIN_Y;
			if(rend.x > ROAD_MAX_X)
				rend.x = ROAD_MAX_X;
			if(rend.y > ROAD_MAX_Y)
				rend.y = ROAD_MAX_Y;
			Int2 dir = rend - rstart;
			float ratio = (Random() + Random()) / 2;
			if(dir.x)
				pt = Int2(rstart.x + ROAD_MIN_DIST + (dir.x - ROAD_MIN_DIST * 2) * ratio, rstart.y);
			else
				pt = Int2(rstart.x, rstart.y + ROAD_MIN_DIST + (dir.y - ROAD_MIN_DIST * 2) * ratio);

			choices = 2;
			if(horizontal)
			{
				choice[0] = GDIR_UP;
				choice[1] = GDIR_DOWN;
			}
			else
			{
				choice[0] = GDIR_LEFT;
				choice[1] = GDIR_RIGHT;
			}
		}
		else
		{
			choices = 3;
			if(type == RT_START)
			{
				r.flags |= ROAD_START_CHECKED;
				pt = r.start;
			}
			else
			{
				r.flags |= ROAD_END_CHECKED;
				pt = r.end;
			}
			if(horizontal)
			{
				choice[0] = GDIR_UP;
				choice[1] = GDIR_DOWN;
				choice[2] = (type == RT_START ? GDIR_LEFT : GDIR_RIGHT);
			}
			else
			{
				choice[0] = GDIR_LEFT;
				choice[1] = GDIR_RIGHT;
				choice[2] = (type == RT_END ? GDIR_DOWN : GDIR_UP);
			}
		}

		GameDirection dir = (GameDirection)get_choice_pop(choice, choices);
		const bool all_done = IsAllSet(r.flags, ROAD_ALL_CHECKED);
		bool try_dual;
		if(MakeAndFillRoad(pt, dir, index))
			try_dual = ((Rand() % ROAD_DUAL_CHANCE) == 0);
		else
			try_dual = ((Rand() % ROAD_DUAL_CHANCE_IF_FAIL) == 0);

		if(try_dual)
		{
			dir = (GameDirection)get_choice_pop(choice, choices);
			MakeAndFillRoad(pt, dir, index);
			if(choices && (Rand() % ROAD_TRIPLE_CHANCE) == 0)
				MakeAndFillRoad(pt, (GameDirection)choice[0], index);
		}

		if(!all_done)
			to_check.push_back(index);
	}

	to_check.clear();
}

//=================================================================================================
int CityGenerator::MakeRoad(const Int2& start_pt, GameDirection dir, int road_index, int& collided_road)
{
	collided_road = -1;

	Int2 pt = start_pt, prev_pt;
	bool horizontal = (dir == GDIR_LEFT || dir == GDIR_RIGHT);
	int dist = 0,
		minx = int(CITY_BORDER_MIN*w),
		miny = int(CITY_BORDER_MIN*h),
		maxx = int(CITY_BORDER_MAX*w),
		maxy = int(CITY_BORDER_MAX*h);
	if(horizontal)
	{
		miny += ROAD_CHECK;
		maxy -= ROAD_CHECK;
	}
	else
	{
		minx += ROAD_CHECK;
		maxx -= ROAD_CHECK;
	}

	while(true)
	{
		prev_pt = pt;
		switch(dir)
		{
		case GDIR_LEFT:
			--pt.x;
			break;
		case GDIR_RIGHT:
			++pt.x;
			break;
		case GDIR_DOWN:
			--pt.y;
			break;
		case GDIR_UP:
			++pt.y;
			break;
		}
		++dist;

		// check city extents
		if(pt.x <= minx || pt.x >= maxx || pt.y <= miny || pt.y >= maxy)
		{
			--dist;
			pt = prev_pt;
			break;
		}

		// check other roads collisions
		bool ok = true;
		Int2 imod;
		if(dir == GDIR_LEFT || dir == GDIR_RIGHT)
			imod = Int2(0, 1);
		else
			imod = Int2(1, 0);
		for(int i = -ROAD_CHECK; i <= ROAD_CHECK; ++i)
		{
			int j = pt.x + i * imod.x + (pt.y + i * imod.y)*w;
			int road_index2 = road_ids[j];
			if(tiles[j].mode != TM_NORMAL && road_index2 != road_index)
			{
				collided_road = road_index2;
				ok = false;
				break;
			}
		}

		if(!ok)
		{
			--dist;
			pt = prev_pt;
			break;
		}
	}

	return dist;
}

//=================================================================================================
void CityGenerator::FillRoad(const Int2& pt, GameDirection dir, int dist)
{
	int index = (int)roads.size();
	Road2& road = Add1(roads);
	Int2 start_pt = pt, end_pt = pt;
	switch(dir)
	{
	case GDIR_LEFT:
		start_pt.x -= dist;
		road.flags = ROAD_HORIZONTAL | ROAD_END_CHECKED;
		break;
	case GDIR_RIGHT:
		end_pt.x += dist;
		road.flags = ROAD_HORIZONTAL | ROAD_START_CHECKED;
		break;
	case GDIR_DOWN:
		start_pt.y -= dist;
		road.flags = ROAD_END_CHECKED;
		break;
	case GDIR_UP:
		end_pt.y += dist;
		road.flags = ROAD_START_CHECKED;
		break;
	}

	road.start = start_pt;
	road.end = end_pt;

	int minx = start_pt.x,
		miny = start_pt.y,
		maxx = end_pt.x,
		maxy = end_pt.y;

	if(IsSet(road.flags, ROAD_HORIZONTAL))
	{
		--minx;
		++maxx;
		miny -= ROAD_WIDTH;
		maxy += ROAD_WIDTH;
	}
	else
	{
		minx -= ROAD_WIDTH;
		maxx += ROAD_WIDTH;
		--miny;
		++maxy;
	}

	for(int y = miny; y <= maxy; ++y)
	{
		for(int x = minx; x <= maxx; ++x)
		{
			int j = x + y * w;
			if(tiles[j].mode != TM_ROAD)
				tiles[j].Set(road_tile, TM_ROAD);
			int& road_id = road_ids[j];
			if(road_id == -1)
				road_id = index;
		}
	}

	if(road.Length() < ROAD_MIN_MID_SPLIT)
		road.flags |= ROAD_MID_CHECKED;

	to_check.push_back(index);
}

//=================================================================================================
bool CityGenerator::MakeAndFillRoad(const Int2& pt, GameDirection dir, int road_index)
{
	int collided_road;
	int road_dist = MakeRoad(pt, dir, road_index, collided_road);
	if(collided_road != -1)
	{
		if(Rand() % ROAD_JOIN_CHANCE == 0)
			++road_dist;
		else
		{
			road_dist -= ROAD_CHECK;
			collided_road = -1;
		}
	}
	if(road_dist >= ROAD_MIN_DIST)
	{
		if(collided_road == -1)
			road_dist = (Random(ROAD_MIN_DIST, road_dist) + Random(ROAD_MIN_DIST, road_dist)) / 2;
		FillRoad(pt, dir, road_dist);
		return true;
	}
	else
		return false;
}

//=================================================================================================
// used to debug wrong tiles in city generation
void CityGenerator::CheckTiles(TERRAIN_TILE t)
{
	for(int y = 0; y < h; ++y)
	{
		for(int x = 0; x < w; ++x)
		{
			TerrainTile& tt = tiles[x + y * w];
			if(tt.t == t || tt.t2 == t)
			{
				assert(0);
			}
		}
	}
}

//=================================================================================================
void CityGenerator::Test()
{
	const int swaps[RoadType_Max] = {
		2,
		1,
		2,
		1,
		6,
		1,
		1
	};

	int s = 128;
	TerrainTile* tiles = new TerrainTile[s*s];
	float* h = new float[(s + 1)*(s + 1)];
	vector<EntryPoint> points;
	int gates;
	SetRoadSize(3, 32);

	for(int i = 0; i < (int)RoadType_Max; ++i)
	{
		for(int j = 0; j < 4; ++j)
		{
			for(int k = 0; k < 4; ++k)
			{
				for(int l = 0; l < swaps[i]; ++l)
				{
					for(int m = 0; m < 2; ++m)
					{
						Info("Test road %d %d %d %d %d", i, j, k, l, m);
						Init(tiles, h, s, s);
						GenerateMainRoad((RoadType)i, (GameDirection)j, k, m == 0 ? false : true, l, points, gates, false);
						assert(_CrtCheckMemory());
						points.clear();
					}
				}
			}
		}
	}

	delete[] tiles;
	delete[] h;
}

//=================================================================================================
bool CityGenerator::IsPointNearRoad(int x, int y)
{
	if(x > 0 && y > 0)
	{
		if(tiles[x - 1 + (y - 1)*w].mode == TM_ROAD)
			return true;
	}

	if(x != w && y > 0)
	{
		if(tiles[x + (y - 1)*w].mode == TM_ROAD)
			return true;
	}

	if(x > 0 && y != h)
	{
		if(tiles[x - 1 + y * w].mode == TM_ROAD)
			return true;
	}

	if(x != w && y != h)
	{
		if(tiles[x + y * w].mode == TM_ROAD)
			return true;
	}

	return false;
}

//=================================================================================================
int CityGenerator::GetNumberOfSteps()
{
	int steps = LocationGenerator::GetNumberOfSteps();
	if(first)
		steps += 4; // txGeneratingBuildings, txGeneratingObjects, txGeneratingUnits, txGeneratingItems
	else if(!reenter)
	{
		steps += 2; // txGeneratingUnits, txGeneratingPhysics
		if(loc->last_visit != world->GetWorldtime())
			++steps; // txGeneratingItems
	}
	if(!reenter)
		++steps; // txRecreatingObjects
	return steps;
}

//=================================================================================================
void CityGenerator::Generate()
{
	bool village = city->IsVillage();
	if(village)
	{
		Info("Generating village map.");
		ClearBit(city->flags, City::HaveExit);
	}
	else
		Info("Generating city map.");

	CreateMap();
	Init(city->tiles, city->h, OutsideLocation::size, OutsideLocation::size);
	SetRoadSize(3, 32);
	float hmax = village ? Random(5.f, 12.f) : Random(2.5f, 5.f);
	int octaves = Random(2, 8);
	float frequency = Random(3.f, 16.f);
	RandomizeHeight(octaves, frequency, 0.f, hmax);

	vector<ToBuild> tobuild;
	if(village)
	{
		RoadType rtype;
		int roads, swap = 0;
		bool plaza;
		GameDirection dir = (GameDirection)(Rand() % 4);
		bool extra_roads;

		switch(Rand() % 6)
		{
		case 0:
			rtype = RoadType_Line;
			roads = Random(0, 2);
			plaza = (Rand() % 3 == 0);
			extra_roads = true;
			break;
		case 1:
			rtype = RoadType_Curve;
			roads = (Rand() % 4 == 0 ? 1 : 0);
			plaza = false;
			extra_roads = false;
			break;
		case 2:
			rtype = RoadType_Oval;
			roads = (Rand() % 4 == 0 ? 1 : 0);
			plaza = false;
			extra_roads = false;
			break;
		case 3:
			rtype = RoadType_Three;
			roads = Random(0, 3);
			plaza = (Rand() % 3 == 0);
			swap = Rand() % 6;
			extra_roads = true;
			break;
		case 4:
			rtype = RoadType_Sin;
			roads = (Rand() % 4 == 0 ? 1 : 0);
			plaza = (Rand() % 3 == 0);
			extra_roads = false;
			break;
		case 5:
			rtype = RoadType_Part;
			roads = (Rand() % 3 == 0 ? 1 : 0);
			plaza = (Rand() % 3 != 0);
			extra_roads = true;
			break;
		}

		GenerateMainRoad(rtype, dir, roads, plaza, swap, city->entry_points, city->gates, extra_roads);
		if(extra_roads)
			GenerateRoads(TT_SAND, 5);
		FlattenRoadExits();
		for(int i = 0; i < 2; ++i)
			FlattenRoad();

		city->PrepareCityBuildings(tobuild);

		GenerateBuildings(tobuild);
		GenerateFields();
		SmoothTerrain();
		FlattenRoad();

		have_well = false;
	}
	else
	{
		RoadType rtype;
		int swap = 0;
		bool plaza = (Rand() % 2 == 0);
		GameDirection dir = (GameDirection)(Rand() % 4);

		switch(Rand() % 4)
		{
		case 0:
			rtype = RoadType_Plus;
			break;
		case 1:
			rtype = RoadType_Line;
			break;
		case 2:
		case 3:
			rtype = RoadType_Three;
			swap = Rand() % 6;
			break;
		}

		GenerateMainRoad(rtype, dir, 4, plaza, swap, city->entry_points, city->gates, true);
		FlattenRoadExits();
		GenerateRoads(TT_ROAD, 25);
		for(int i = 0; i < 2; ++i)
			FlattenRoad();

		city->PrepareCityBuildings(tobuild);

		GenerateBuildings(tobuild);
		ApplyWallTiles(city->gates);

		SmoothTerrain();
		FlattenRoad();

		if(plaza && Rand() % 4 != 0)
		{
			have_well = true;
			well_pt = Int2(64, 64);
		}
		else
			have_well = false;
	}

	// budynki
	city->buildings.resize(tobuild.size());
	vector<ToBuild>::iterator build_it = tobuild.begin();
	for(vector<CityBuilding>::iterator it = city->buildings.begin(), end = city->buildings.end(); it != end; ++it, ++build_it)
	{
		it->building = build_it->building;
		it->pt = build_it->pt;
		it->rot = build_it->rot;
		it->unit_pt = build_it->unit_pt;
	}

	if(!village)
	{
		// set exits y
		terrain->SetHeightMap(city->h);
		for(vector<EntryPoint>::iterator entry_it = city->entry_points.begin(), entry_end = city->entry_points.end(); entry_it != entry_end; ++entry_it)
			entry_it->exit_y = terrain->GetH(entry_it->exit_region.Midpoint()) + 0.1f;
		terrain->RemoveHeightMap();
	}

	CleanBorders();
}

//=================================================================================================
void CityGenerator::OnEnter()
{
	game->arena->free = true;

	if(!reenter)
	{
		game_level->Apply();
		ApplyTiles();
	}

	game_level->SetOutsideParams();

	if(first)
	{
		// generate buildings
		game->LoadingStep(game->txGeneratingBuildings);
		SpawnBuildings();

		// generate objects
		game->LoadingStep(game->txGeneratingObjects);
		SpawnObjects();

		// generate units
		game->LoadingStep(game->txGeneratingUnits);
		SpawnUnits();
		SpawnTemporaryUnits();

		// generate items
		game->LoadingStep(game->txGeneratingItems);
		GeneratePickableItems();
		if(city->IsVillage())
			SpawnForestItems(-2);
	}
	else if(!reenter)
	{
		// remove temporary/quest units
		if(city->reset)
			RemoveTemporaryUnits();

		// remove blood/corpses
		int days;
		city->CheckUpdate(days, world->GetWorldtime());
		if(days > 0)
			game_level->UpdateLocation(days, 100, false);

		// recreate units
		game->LoadingStep(game->txGeneratingUnits);
		RespawnUnits();
		RepositionUnits();

		// recreate physics
		game->LoadingStep(game->txGeneratingPhysics);
		game_level->RecreateObjects();
		RespawnBuildingPhysics();

		if(city->reset)
		{
			SpawnTemporaryUnits();
			city->reset = false;
		}

		// generate items
		if(days > 0)
		{
			game->LoadingStep(game->txGeneratingItems);
			if(days >= 10)
			{
				GeneratePickableItems();
				if(city->IsVillage())
					SpawnForestItems(-2);
			}
		}

		game_level->OnReenterLevel();
	}

	if(!reenter)
	{
		// create colliders
		game->LoadingStep(game->txRecreatingObjects);
		game_level->SpawnTerrainCollider();
		SpawnCityPhysics();
		SpawnOutsideBariers();
		for(InsideBuilding* b : city->inside_buildings)
		{
			b->mine = Int2(b->level_shift.x * 256, b->level_shift.y * 256);
			b->maxe = b->mine + Int2(256, 256);
		}
	}

	// spawn quest units
	if(game_level->location->active_quest && game_level->location->active_quest != (Quest_Dungeon*)ACTIVE_QUEST_HOLDER && !game_level->location->active_quest->done
		&& game_level->location->active_quest->type != Q_SCRIPTED)
		game->HandleQuestEvent(game_level->location->active_quest);

	// generate minimap
	game->LoadingStep(game->txGeneratingMinimap);
	CreateMinimap();

	// dodaj gracza i jego dru¿ynê
	Vec3 spawn_pos;
	float spawn_dir;
	city->GetEntry(spawn_pos, spawn_dir);
	game_level->AddPlayerTeam(spawn_pos, spawn_dir, reenter, true);

	if(!reenter)
		game->GenerateQuestUnits();

	for(Unit& unit : team->members)
	{
		if(unit.IsHero())
			unit.hero->lost_pvp = false;
	}

	team->CheckTeamItemShares();

	Quest_Contest* contest = quest_mgr->quest_contest;
	if(!contest->generated && game_level->location_index == contest->where && contest->state == Quest_Contest::CONTEST_TODAY)
		contest->SpawnDrunkmans();
}

//=================================================================================================
void CityGenerator::SpawnBuildings()
{
	LevelArea& area = *city;
	const int mur1 = int(0.15f*OutsideLocation::size);
	const int mur2 = int(0.85f*OutsideLocation::size);

	// budynki
	for(vector<CityBuilding>::iterator it = city->buildings.begin(), end = city->buildings.end(); it != end; ++it)
	{
		Object* o = new Object;

		switch(it->rot)
		{
		case GDIR_DOWN:
			o->rot.y = 0.f;
			break;
		case GDIR_LEFT:
			o->rot.y = PI / 2;
			break;
		case GDIR_UP:
			o->rot.y = PI;
			break;
		case GDIR_RIGHT:
			o->rot.y = PI * 3 / 2;
			break;
		}

		o->pos = Vec3(float(it->pt.x + it->building->shift[it->rot].x) * 2, 1.f, float(it->pt.y + it->building->shift[it->rot].y) * 2);
		terrain->SetH(o->pos);
		o->rot.x = o->rot.z = 0.f;
		o->scale = 1.f;
		o->base = nullptr;
		o->mesh = it->building->mesh;
		area.objects.push_back(o);
	}

	// create walls, towers & gates
	if(!city->IsVillage())
	{
		const int mid = int(0.5f*OutsideLocation::size);
		BaseObject* oWall = BaseObject::Get("wall"),
			*oTower = BaseObject::Get("tower"),
			*oGate = BaseObject::Get("gate"),
			*oGrate = BaseObject::Get("grate");

		// walls
		for(int i = mur1; i < mur2; i += 3)
		{
			// north
			if(!IsSet(city->gates, GATE_NORTH) || i < mid - 1 || i > mid)
				game_level->SpawnObjectEntity(area, oWall, Vec3(float(i) * 2 + 1.f, 1.f, int(0.85f*OutsideLocation::size) * 2 + 1.f), 0);

			// south
			if(!IsSet(city->gates, GATE_SOUTH) || i < mid - 1 || i > mid)
				game_level->SpawnObjectEntity(area, oWall, Vec3(float(i) * 2 + 1.f, 1.f, int(0.15f*OutsideLocation::size) * 2 + 1.f), PI);

			// west
			if(!IsSet(city->gates, GATE_WEST) || i < mid - 1 || i > mid)
				game_level->SpawnObjectEntity(area, oWall, Vec3(int(0.15f*OutsideLocation::size) * 2 + 1.f, 1.f, float(i) * 2 + 1.f), PI * 3 / 2);

			// east
			if(!IsSet(city->gates, GATE_EAST) || i < mid - 1 || i > mid)
				game_level->SpawnObjectEntity(area, oWall, Vec3(int(0.85f*OutsideLocation::size) * 2 + 1.f, 1.f, float(i) * 2 + 1.f), PI / 2);
		}

		// towers
		// north east
		game_level->SpawnObjectEntity(area, oTower, Vec3(int(0.85f*OutsideLocation::size) * 2 + 1.f, 1.f, int(0.85f*OutsideLocation::size) * 2 + 1.f), 0);
		// south east
		game_level->SpawnObjectEntity(area, oTower, Vec3(int(0.85f*OutsideLocation::size) * 2 + 1.f, 1.f, int(0.15f*OutsideLocation::size) * 2 + 1.f), PI / 2);
		// south west
		game_level->SpawnObjectEntity(area, oTower, Vec3(int(0.15f*OutsideLocation::size) * 2 + 1.f, 1.f, int(0.15f*OutsideLocation::size) * 2 + 1.f), PI);
		// north west
		game_level->SpawnObjectEntity(area, oTower, Vec3(int(0.15f*OutsideLocation::size) * 2 + 1.f, 1.f, int(0.85f*OutsideLocation::size) * 2 + 1.f), PI * 3 / 2);

		// gates
		if(IsSet(city->gates, GATE_NORTH))
		{
			game_level->SpawnObjectEntity(area, oGate, Vec3(0.5f*OutsideLocation::size * 2 + 1.f, 1.f, 0.85f*OutsideLocation::size * 2), 0);
			game_level->SpawnObjectEntity(area, oGrate, Vec3(0.5f*OutsideLocation::size * 2 + 1.f, 1.f, 0.85f*OutsideLocation::size * 2), 0);
		}

		if(IsSet(city->gates, GATE_SOUTH))
		{
			game_level->SpawnObjectEntity(area, oGate, Vec3(0.5f*OutsideLocation::size * 2 + 1.f, 1.f, 0.15f*OutsideLocation::size * 2), PI);
			game_level->SpawnObjectEntity(area, oGrate, Vec3(0.5f*OutsideLocation::size * 2 + 1.f, 1.f, 0.15f*OutsideLocation::size * 2), PI);
		}

		if(IsSet(city->gates, GATE_WEST))
		{
			game_level->SpawnObjectEntity(area, oGate, Vec3(0.15f*OutsideLocation::size * 2, 1.f, 0.5f*OutsideLocation::size * 2 + 1.f), PI * 3 / 2);
			game_level->SpawnObjectEntity(area, oGrate, Vec3(0.15f*OutsideLocation::size * 2, 1.f, 0.5f*OutsideLocation::size * 2 + 1.f), PI * 3 / 2);
		}

		if(IsSet(city->gates, GATE_EAST))
		{
			game_level->SpawnObjectEntity(area, oGate, Vec3(0.85f*OutsideLocation::size * 2, 1.f, 0.5f*OutsideLocation::size * 2 + 1.f), PI / 2);
			game_level->SpawnObjectEntity(area, oGrate, Vec3(0.85f*OutsideLocation::size * 2, 1.f, 0.5f*OutsideLocation::size * 2 + 1.f), PI / 2);
		}
	}

	// obiekty i fizyka budynków
	for(vector<CityBuilding>::iterator it = city->buildings.begin(), end = city->buildings.end(); it != end; ++it)
	{
		Building* b = it->building;
		game_level->ProcessBuildingObjects(area, city, nullptr, b->mesh, b->inside_mesh, DirToRot(it->rot), it->rot,
			Vec3(float(it->pt.x + b->shift[it->rot].x) * 2, 0.f, float(it->pt.y + b->shift[it->rot].y) * 2), b, &*it);
	}
}

//=================================================================================================
OutsideObject outside_objects[] = {
	"tree", nullptr, Vec2(3,5),
	"tree2", nullptr, Vec2(3,5),
	"tree3", nullptr, Vec2(3,5),
	"grass", nullptr, Vec2(1.f,1.5f),
	"grass", nullptr, Vec2(1.f,1.5f),
	"plant", nullptr, Vec2(1.0f,2.f),
	"plant", nullptr, Vec2(1.0f,2.f),
	"rock", nullptr, Vec2(1.f,1.f),
	"fern", nullptr, Vec2(1,2)
};
const uint n_outside_objects = countof(outside_objects);

void CityGenerator::SpawnObjects()
{
	LevelArea& area = *city;

	if(!outside_objects[0].obj)
	{
		for(uint i = 0; i < n_outside_objects; ++i)
			outside_objects[i].obj = BaseObject::Get(outside_objects[i].name);
	}

	// well
	if(have_well)
	{
		Vec3 pos = PtToPos(well_pt);
		terrain->SetH(pos);
		game_level->SpawnObjectEntity(area, BaseObject::Get("coveredwell"), pos, PI / 2 * (Rand() % 4), 1.f, 0, nullptr);
	}

	TerrainTile* tiles = city->tiles;

	for(int i = 0; i < 512; ++i)
	{
		Int2 pt(Random(1, OutsideLocation::size - 2), Random(1, OutsideLocation::size - 2));
		if(tiles[pt.x + pt.y*OutsideLocation::size].mode == TM_NORMAL
			&& tiles[pt.x - 1 + pt.y*OutsideLocation::size].mode == TM_NORMAL
			&& tiles[pt.x + 1 + pt.y*OutsideLocation::size].mode == TM_NORMAL
			&& tiles[pt.x + (pt.y - 1)*OutsideLocation::size].mode == TM_NORMAL
			&& tiles[pt.x + (pt.y + 1)*OutsideLocation::size].mode == TM_NORMAL
			&& tiles[pt.x - 1 + (pt.y - 1)*OutsideLocation::size].mode == TM_NORMAL
			&& tiles[pt.x - 1 + (pt.y + 1)*OutsideLocation::size].mode == TM_NORMAL
			&& tiles[pt.x + 1 + (pt.y - 1)*OutsideLocation::size].mode == TM_NORMAL
			&& tiles[pt.x + 1 + (pt.y + 1)*OutsideLocation::size].mode == TM_NORMAL)
		{
			Vec3 pos(Random(2.f) + 2.f*pt.x, 0, Random(2.f) + 2.f*pt.y);
			pos.y = terrain->GetH(pos);
			OutsideObject& o = outside_objects[Rand() % n_outside_objects];
			game_level->SpawnObjectEntity(area, o.obj, pos, Random(MAX_ANGLE), o.scale.Random());
		}
	}
}

//=================================================================================================
void CityGenerator::SpawnUnits()
{
	LevelArea& area = *city;

	for(CityBuilding& b : city->buildings)
	{
		UnitData* ud = b.building->unit;
		if(!ud)
			continue;

		Unit* u = game->CreateUnit(*ud, -2);
		u->area = city;

		switch(b.rot)
		{
		case GDIR_DOWN:
			u->rot = 0.f;
			break;
		case GDIR_LEFT:
			u->rot = PI / 2;
			break;
		case GDIR_UP:
			u->rot = PI;
			break;
		case GDIR_RIGHT:
			u->rot = PI * 3 / 2;
			break;
		}

		u->pos = Vec3(float(b.unit_pt.x) * 2 + 1, 0, float(b.unit_pt.y) * 2 + 1);
		terrain->SetH(u->pos);
		u->UpdatePhysics();
		u->visual_pos = u->pos;

		if(b.building->group == BuildingGroup::BG_ARENA)
			city->arena_pos = u->pos;

		area.units.push_back(u);

		AIController* ai = new AIController;
		ai->Init(u);
		game->ais.push_back(ai);
	}

	UnitData* dweller = UnitData::Get(city->IsVillage() ? "villager" : "citizen");

	// pijacy w karczmie
	for(int i = 0, count = Random(1, city->citizens / 3); i < count; ++i)
	{
		if(!game_level->SpawnUnitInsideInn(*dweller, -2))
			break;
	}

	// wêdruj¹cy mieszkañcy
	const int a = int(0.15f*OutsideLocation::size) + 3;
	const int b = int(0.85f*OutsideLocation::size) - 3;

	for(int i = 0; i < city->citizens; ++i)
	{
		for(int j = 0; j < 50; ++j)
		{
			Int2 pt(Random(a, b), Random(a, b));
			if(city->tiles[pt(OutsideLocation::size)].IsRoadOrPath())
			{
				game_level->SpawnUnitNearLocation(area, Vec3(2.f*pt.x + 1, 0, 2.f*pt.y + 1), *dweller, nullptr, -2, 2.f);
				break;
			}
		}
	}

	// stra¿nicy
	UnitData* guard = UnitData::Get("guard_move");
	for(int i = 0, count = city->IsVillage() ? 3 : 6; i < count; ++i)
	{
		for(int j = 0; j < 50; ++j)
		{
			Int2 pt(Random(a, b), Random(a, b));
			if(city->tiles[pt(OutsideLocation::size)].IsRoadOrPath())
			{
				game_level->SpawnUnitNearLocation(area, Vec3(2.f*pt.x + 1, 0, 2.f*pt.y + 1), *guard, nullptr, -2, 2.f);
				break;
			}
		}
	}
}

//=================================================================================================
void CityGenerator::SpawnTemporaryUnits()
{
	InsideBuilding* inn = city->FindInn();
	CityBuilding* training_grounds = city->FindBuilding(BuildingGroup::BG_TRAINING_GROUNDS);

	// heroes
	uint count;
	Int2 level;
	if(world->CheckFirstCity())
	{
		count = 4;
		level = Int2(5, 8);
	}
	else
	{
		count = Random(1u, 4u);
		level = Int2(5, 15);
	}

	for(uint i = 0; i < count; ++i)
	{
		UnitData& ud = *Class::GetRandomHeroData();

		if(Rand() % 2 == 0 || !training_grounds)
		{
			// inside inn
			game_level->SpawnUnitInsideInn(ud, level.Random(), inn, true);
		}
		else
		{
			// on training grounds
			Unit* u = game_level->SpawnUnitNearLocation(*city, Vec3(2.f*training_grounds->unit_pt.x + 1, 0, 2.f*training_grounds->unit_pt.y + 1), ud, nullptr,
				level.Random(), 8.f);
			if(u)
				u->temporary = true;
		}
	}

	// quest traveler (100% chance in city, 50% in village)
	if(!city->IsVillage() || Rand() % 2 == 0)
		game_level->SpawnUnitInsideInn(*UnitData::Get("traveler"), -2, inn, Level::SU_TEMPORARY);
}

//=================================================================================================
void CityGenerator::RemoveTemporaryUnits()
{
	for(LevelArea& area : game_level->ForEachArea())
		DeleteElements(area.units, [](Unit* u) {return u->temporary; });
}

//=================================================================================================
void CityGenerator::RepositionUnits()
{
	const int a = int(0.15f*OutsideLocation::size) + 3;
	const int b = int(0.85f*OutsideLocation::size) - 3;

	UnitData* citizen;
	if(city->IsVillage())
		citizen = UnitData::Get("villager");
	else
		citizen = UnitData::Get("citizen");
	UnitData* guard = UnitData::Get("guard_move");

	for(vector<Unit*>::iterator it = city->units.begin(), end = city->units.end(); it != end; ++it)
	{
		Unit& u = **it;
		if(u.IsAlive() && u.IsAI() && u.data == citizen || u.data == guard)
		{
			for(int j = 0; j < 50; ++j)
			{
				Int2 pt(Random(a, b), Random(a, b));
				if(city->tiles[pt(OutsideLocation::size)].IsRoadOrPath())
				{
					game_level->WarpUnit(u, Vec3(2.f*pt.x + 1, 0, 2.f*pt.y + 1));
					break;
				}
			}
		}
	}
}

//=================================================================================================
void CityGenerator::GeneratePickableItems()
{
	BaseObject* table = BaseObject::Get("table"),
		*shelves = BaseObject::Get("shelves");

	// alcohol in inn
	InsideBuilding& inn = *city->FindInn();
	const Item* beer = Item::Get("beer");
	const Item* vodka = Item::Get("vodka");
	const Item* plate = Item::Get("plate");
	const Item* cup = Item::Get("cup");
	for(vector<Object*>::iterator it = inn.objects.begin(), end = inn.objects.end(); it != end; ++it)
	{
		Object& obj = **it;
		if(obj.base == table)
		{
			game_level->PickableItemBegin(inn, obj);
			if(Rand() % 2 == 0)
			{
				game_level->PickableItemAdd(beer);
				if(Rand() % 4 == 0)
					game_level->PickableItemAdd(beer);
			}
			if(Rand() % 3 == 0)
				game_level->PickableItemAdd(plate);
			if(Rand() % 3 == 0)
				game_level->PickableItemAdd(cup);
		}
		else if(obj.base == shelves)
		{
			game_level->PickableItemBegin(inn, obj);
			for(int i = 0, count = Random(3, 5); i < count; ++i)
				game_level->PickableItemAdd(beer);
			for(int i = 0, count = Random(1, 3); i < count; ++i)
				game_level->PickableItemAdd(vodka);
		}
	}

	// food in food shop
	CityBuilding* food = city->FindBuilding(BuildingGroup::BG_FOOD_SELLER);
	if(food)
	{
		Object* found_obj = nullptr;
		float best_dist = 9999.f, dist;
		for(vector<Object*>::iterator it = city->objects.begin(), end = city->objects.end(); it != end; ++it)
		{
			Object& obj = **it;
			if(obj.base == shelves)
			{
				dist = Vec3::Distance(food->walk_pt, obj.pos);
				if(dist < best_dist)
				{
					best_dist = dist;
					found_obj = &obj;
				}
			}
		}

		if(found_obj)
		{
			const ItemList* lis = ItemList::Get("food_and_drink").lis;
			game_level->PickableItemBegin(*city, *found_obj);
			for(int i = 0; i < 20; ++i)
				game_level->PickableItemAdd(lis->Get());
		}
	}

	// potions in alchemy shop
	CityBuilding* alch = city->FindBuilding(BuildingGroup::BG_ALCHEMIST);
	if(alch)
	{
		Object* found_obj = nullptr;
		float best_dist = 9999.f, dist;
		for(vector<Object*>::iterator it = city->objects.begin(), end = city->objects.end(); it != end; ++it)
		{
			Object& obj = **it;
			if(obj.base == shelves)
			{
				dist = Vec3::Distance(alch->walk_pt, obj.pos);
				if(dist < best_dist)
				{
					best_dist = dist;
					found_obj = &obj;
				}
			}
		}

		if(found_obj)
		{
			game_level->PickableItemBegin(*city, *found_obj);
			const Item* heal_pot = Item::Get("p_hp");
			game_level->PickableItemAdd(heal_pot);
			if(Rand() % 2 == 0)
				game_level->PickableItemAdd(heal_pot);
			if(Rand() % 2 == 0)
				game_level->PickableItemAdd(Item::Get("p_nreg"));
			if(Rand() % 2 == 0)
				game_level->PickableItemAdd(Item::Get("healing_herb"));
			if(Rand() % 2 == 0)
				game_level->PickableItemAdd(Item::Get("p_mp"));
		}
	}
}

//=================================================================================================
void CityGenerator::CreateMinimap()
{
	TextureLock lock(game->tMinimap.tex);

	for(int y = 0; y < OutsideLocation::size; ++y)
	{
		uint* pix = lock[y];
		for(int x = 0; x < OutsideLocation::size; ++x)
		{
			const TerrainTile& t = city->tiles[x + (OutsideLocation::size - 1 - y)*OutsideLocation::size];
			Color col;
			if(t.mode >= TM_BUILDING)
				col = Color(128, 64, 0);
			else if(t.alpha == 0)
			{
				if(t.t == TT_GRASS)
					col = Color(0, 128, 0);
				else if(t.t == TT_ROAD)
					col = Color(128, 128, 128);
				else if(t.t == TT_FIELD)
					col = Color(200, 200, 100);
				else
					col = Color(128, 128, 64);
			}
			else
			{
				int r, g, b, r2, g2, b2;
				switch(t.t)
				{
				case TT_GRASS:
					r = 0;
					g = 128;
					b = 0;
					break;
				case TT_ROAD:
					r = 128;
					g = 128;
					b = 128;
					break;
				case TT_FIELD:
					r = 200;
					g = 200;
					b = 0;
					break;
				case TT_SAND:
				default:
					r = 128;
					g = 128;
					b = 64;
					break;
				}
				switch(t.t2)
				{
				case TT_GRASS:
					r2 = 0;
					g2 = 128;
					b2 = 0;
					break;
				case TT_ROAD:
					r2 = 128;
					g2 = 128;
					b2 = 128;
					break;
				case TT_FIELD:
					r2 = 200;
					g2 = 200;
					b2 = 0;
					break;
				case TT_SAND:
				default:
					r2 = 128;
					g2 = 128;
					b2 = 64;
					break;
				}
				const float T = float(t.alpha) / 255;
				col = Color(Lerp(r, r2, T), Lerp(g, g2, T), Lerp(b, b2, T));
			}
			if(x < 16 || x > 128 - 16 || y < 16 || y > 128 - 16)
			{
				col.r /= 2;
				col.g /= 2;
				col.b /= 2;
			}
			*pix = col;
			++pix;
		}
	}

	game_level->minimap_size = OutsideLocation::size;
}

//=================================================================================================
void CityGenerator::OnLoad()
{
	game_level->SetOutsideParams();
	game->SetTerrainTextures();
	ApplyTiles();

	game_level->RecreateObjects(Net::IsClient());
	game_level->SpawnTerrainCollider();
	RespawnBuildingPhysics();
	SpawnCityPhysics();
	SpawnOutsideBariers();
	game->InitQuadTree();
	game->CalculateQuadtree();

	CreateMinimap();
}

//=================================================================================================
void CityGenerator::SpawnCityPhysics()
{
	TerrainTile* tiles = city->tiles;

	for(int z = 0; z < City::size; ++z)
	{
		for(int x = 0; x < City::size; ++x)
		{
			if(tiles[x + z * OutsideLocation::size].mode == TM_BUILDING_BLOCK)
			{
				btCollisionObject* cobj = new btCollisionObject;
				cobj->setCollisionShape(game_level->shape_block);
				cobj->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_BUILDING);
				cobj->getWorldTransform().setOrigin(btVector3(2.f*x + 1.f, terrain->GetH(2.f*x + 1.f, 2.f*x + 1), 2.f*z + 1.f));
				game_level->phy_world->addCollisionObject(cobj, CG_BUILDING);
			}
		}
	}
}

//=================================================================================================
void CityGenerator::RespawnBuildingPhysics()
{
	for(vector<CityBuilding>::iterator it = city->buildings.begin(), end = city->buildings.end(); it != end; ++it)
	{
		Building* b = it->building;
		game_level->ProcessBuildingObjects(*city, city, nullptr, b->mesh, nullptr, DirToRot(it->rot), it->rot,
			Vec3(float(it->pt.x + b->shift[it->rot].x) * 2, 1.f, float(it->pt.y + b->shift[it->rot].y) * 2), nullptr, &*it, true);
	}

	for(vector<InsideBuilding*>::iterator it = city->inside_buildings.begin(), end = city->inside_buildings.end(); it != end; ++it)
	{
		game_level->ProcessBuildingObjects(**it, city, *it, (*it)->building->inside_mesh, nullptr, 0.f, 0, Vec3((*it)->offset.x, 0.f, (*it)->offset.y), nullptr,
			nullptr, true);
	}
}
