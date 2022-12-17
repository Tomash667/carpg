#include "Pch.h"
#include "CityGenerator.h"

#include "Arena.h"
#include "AIController.h"
#include "AITeam.h"
#include "City.h"
#include "Game.h"
#include "Level.h"
#include "LevelPart.h"
#include "Location.h"
#include "Object.h"
#include "OutsideObject.h"
#include "Quest.h"
#include "QuestManager.h"
#include "Quest_Contest.h"
#include "Stock.h"
#include "Team.h"
#include "World.h"

#include <Scene.h>
#include <Terrain.h>
#include <Texture.h>

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
	roadIds.clear();
	roadIds.resize(w * h, -1);
}

//=================================================================================================
void CityGenerator::SetRoadSize(int roadSize, int roadPart)
{
	assert(roadSize > 0 && roadPart >= 0);
	this->roadSize = roadSize;
	this->roadPart = roadPart;
	rs1 = roadSize / 2;
	rs2 = roadSize / 2;
}

//=================================================================================================
void CityGenerator::GenerateMainRoad(RoadType type, GameDirection dir, int rocky_roads, bool plaza, int swap, vector<EntryPoint>& entryPoints,
	int& gates, bool fillRoads)
{
	memset(tiles, 0, sizeof(TerrainTile) * w * h);

#define W1 (w/2-rs1)
#define W2 (w/2+rs2)
#define H1 (h/2-rs1)
#define H2 (h/2+rs2)

	switch(type)
	{
	// dir oznacza kt�r�dy idzie g��wna droga, swap kt�ra droga jest druga (swap=0 lewa,d�; swap=1 prawa,g�ra) je�li nie s� tego samego typu pod�o�a
	case RoadType_Plus:
		gates = GATE_NORTH | GATE_SOUTH | GATE_EAST | GATE_WEST;
		if(rocky_roads >= 3 || rocky_roads == 0)
		{
			// plus
			TERRAIN_TILE t = (rocky_roads != 0 ? TT_ROAD : TT_SAND);
			CreateRoadLineLeftRight(t, entryPoints);
			CreateRoadLineBottomTop(t, entryPoints);
		}
		else if(dir == GDIR_LEFT || dir == GDIR_RIGHT)
		{
			CreateRoadLineLeftRight(TT_ROAD, entryPoints);
			if(rocky_roads > 1)
			{
				if(!swap)
				{
					CreateRoadPartBottom(TT_ROAD, entryPoints);
					CreateRoadPartTop(TT_SAND, entryPoints);
				}
				else
				{
					CreateRoadPartBottom(TT_SAND, entryPoints);
					CreateRoadPartTop(TT_ROAD, entryPoints);
				}
			}
			else
			{
				CreateRoadPartBottom(TT_SAND, entryPoints);
				CreateRoadPartTop(TT_SAND, entryPoints);
			}
		}
		else
		{
			CreateRoadLineBottomTop(TT_ROAD, entryPoints);
			if(rocky_roads > 1)
			{
				if(!swap)
				{
					CreateRoadPartLeft(TT_ROAD, entryPoints);
					CreateRoadPartRight(TT_SAND, entryPoints);
				}
				else
				{
					CreateRoadPartLeft(TT_SAND, entryPoints);
					CreateRoadPartRight(TT_ROAD, entryPoints);
				}
			}
			else
			{
				CreateRoadPartLeft(TT_SAND, entryPoints);
				CreateRoadPartRight(TT_SAND, entryPoints);
			}
		}
		if(fillRoads)
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
			if(fillRoads)
			{
				AddRoad(Int2(0, h / 2), Int2(w / 2, h / 2), ROAD_HORIZONTAL | ROAD_START_CHECKED | ROAD_END_CHECKED);
				AddRoad(Int2(w / 2, h / 2), Int2(w, h / 2), ROAD_HORIZONTAL | ROAD_START_CHECKED | ROAD_END_CHECKED);
			}
		}
		else
		{
			gates = GATE_NORTH | GATE_SOUTH;
			if(fillRoads)
			{
				AddRoad(Int2(w / 2, 0), Int2(w / 2, h / 2), ROAD_START_CHECKED | ROAD_END_CHECKED);
				AddRoad(Int2(w / 2, h / 2), Int2(w / 2, h), ROAD_START_CHECKED | ROAD_END_CHECKED);
			}
		}

		if(rocky_roads >= 2 || rocky_roads == 0)
		{
			TERRAIN_TILE t = (rocky_roads != 0 ? TT_ROAD : TT_SAND);
			if(dir == GDIR_LEFT || dir == GDIR_RIGHT)
				CreateRoadLineLeftRight(t, entryPoints);
			else
				CreateRoadLineBottomTop(t, entryPoints);
		}
		else
		{
			CreateRoadCenter(TT_ROAD);
			switch(dir)
			{
			case GDIR_LEFT:
				CreateRoadPartLeft(TT_ROAD, entryPoints);
				CreateRoadPartRight(TT_SAND, entryPoints);
				break;
			case GDIR_RIGHT:
				CreateRoadPartRight(TT_ROAD, entryPoints);
				CreateRoadPartLeft(TT_SAND, entryPoints);
				break;
			case GDIR_DOWN:
				CreateRoadPartBottom(TT_ROAD, entryPoints);
				CreateRoadPartTop(TT_SAND, entryPoints);
				break;
			case GDIR_UP:
				CreateRoadPartTop(TT_ROAD, entryPoints);
				CreateRoadPartBottom(TT_SAND, entryPoints);
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
				CreateRoad(Rect(0, H1, roadPart, H2), t);
				CreateEntry(entryPoints, ED_Left);
				pts[0] = Int2(roadPart, h / 2);
				if(!swap)
				{
					CreateRoad(Rect(W1, 0, W2, roadPart), t);
					CreateEntry(entryPoints, ED_Bottom);
					pts[2] = Int2(w / 2, roadPart);
				}
				else
				{
					CreateRoad(Rect(W1, h - roadPart, W2, h - 1), t);
					CreateEntry(entryPoints, ED_Top);
					pts[2] = Int2(w / 2, h - roadPart);
				}
				break;
			case GDIR_RIGHT:
				CreateRoad(Rect(w - roadPart, H1, w - 1, H2), t);
				CreateEntry(entryPoints, ED_Right);
				pts[0] = Int2(w - roadPart, h / 2);
				if(!swap)
				{
					CreateRoad(Rect(W1, 0, W2, roadPart), t);
					CreateEntry(entryPoints, ED_Bottom);
					pts[2] = Int2(w / 2, roadPart);
				}
				else
				{
					CreateRoad(Rect(W1, h - roadPart, W2, h - 1), t);
					CreateEntry(entryPoints, ED_Top);
					pts[2] = Int2(w / 2, h - roadPart);
				}
				break;
			case GDIR_DOWN:
				CreateRoad(Rect(W1, 0, W2, roadPart), t);
				CreateEntry(entryPoints, ED_Bottom);
				pts[0] = Int2(w / 2, roadPart);
				if(!swap)
				{
					CreateRoad(Rect(0, H1, roadPart, H2), t);
					CreateEntry(entryPoints, ED_Left);
					pts[2] = Int2(roadPart, h / 2);
				}
				else
				{
					CreateRoad(Rect(w - roadPart, H1, w - 1, H2), t);
					CreateEntry(entryPoints, ED_Right);
					pts[2] = Int2(w - roadPart, h / 2);
				}
				break;
			case GDIR_UP:
				CreateRoad(Rect(W1, h - roadPart, W2, h - 1), t);
				CreateEntry(entryPoints, ED_Top);
				pts[0] = Int2(w / 2, h - roadPart);
				if(!swap)
				{
					CreateRoad(Rect(0, H1, roadPart, H2), t);
					CreateEntry(entryPoints, ED_Left);
					pts[2] = Int2(roadPart, h / 2);
				}
				else
				{
					CreateRoad(Rect(w - roadPart, H1, w - 1, H2), t);
					CreateEntry(entryPoints, ED_Right);
					pts[2] = Int2(w - roadPart, h / 2);
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
				CreateRoad(Rect(0, H1, roadPart, H2), t);
				CreateEntry(entryPoints, ED_Left);
				CreateRoad(Rect(w - roadPart, H1, w - 1, H2), t);
				CreateEntry(entryPoints, ED_Right);
				Int2 pts[3];
				pts[0] = Int2(roadPart, h / 2);
				pts[1] = Int2(w / 2, roadPart);
				pts[2] = Int2(w - roadPart, h / 2);
				CreateCurveRoad(pts, 3, t);
				pts[1] = Int2(w / 2, h - roadPart);
				CreateCurveRoad(pts, 3, t);
			}
			else
			{
				gates = GATE_NORTH | GATE_SOUTH;
				CreateRoad(Rect(W1, 0, W2, roadPart), t);
				CreateEntry(entryPoints, ED_Bottom);
				CreateRoad(Rect(W1, h - roadPart, W2, h - 1), t);
				CreateEntry(entryPoints, ED_Top);
				Int2 pts[3];
				pts[0] = Int2(w / 2, roadPart);
				pts[1] = Int2(roadPart, h / 2);
				pts[2] = Int2(w / 2, h - roadPart);
				CreateCurveRoad(pts, 3, t);
				pts[1] = Int2(w - roadPart, h / 2);
				CreateCurveRoad(pts, 3, t);
			}
		}
		break;

		// dir oznacza brakuj�c� drog�, swap kolejno�� (0-5)
	case RoadType_Three:
		if(rocky_roads >= 3 || rocky_roads == 0)
		{
			TERRAIN_TILE t = (rocky_roads != 0 ? TT_ROAD : TT_SAND);
			switch(dir)
			{
			case GDIR_LEFT:
				gates = GATE_NORTH | GATE_SOUTH | GATE_EAST;
				CreateRoadLineBottomTop(t, entryPoints);
				CreateRoadPartRight(t, entryPoints);
				break;
			case GDIR_RIGHT:
				gates = GATE_NORTH | GATE_SOUTH | GATE_WEST;
				CreateRoadLineBottomTop(t, entryPoints);
				CreateRoadPartLeft(t, entryPoints);
				break;
			case GDIR_DOWN:
				gates = GATE_NORTH | GATE_EAST | GATE_WEST;
				CreateRoadLineLeftRight(t, entryPoints);
				CreateRoadPartTop(t, entryPoints);
				break;
			case GDIR_UP:
				gates = GATE_SOUTH | GATE_EAST | GATE_WEST;
				CreateRoadLineLeftRight(t, entryPoints);
				CreateRoadPartBottom(t, entryPoints);
				break;
			}
		}
		else
		{
			constexpr int mod[6][3] = {
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
				CreateRoadPartRight(GetMod(0), entryPoints);
				CreateRoadPartBottom(GetMod(1), entryPoints);
				CreateRoadPartTop(GetMod(2), entryPoints);
				break;
			case GDIR_RIGHT:
				gates = GATE_NORTH | GATE_SOUTH | GATE_WEST;
				CreateRoadPartLeft(GetMod(0), entryPoints);
				CreateRoadPartBottom(GetMod(1), entryPoints);
				CreateRoadPartTop(GetMod(2), entryPoints);
				break;
			case GDIR_DOWN:
				gates = GATE_NORTH | GATE_EAST | GATE_WEST;
				CreateRoadPartTop(GetMod(0), entryPoints);
				CreateRoadPartLeft(GetMod(1), entryPoints);
				CreateRoadPartRight(GetMod(2), entryPoints);
				break;
			case GDIR_UP:
				gates = GATE_SOUTH | GATE_EAST | GATE_WEST;
				CreateRoadPartBottom(GetMod(0), entryPoints);
				CreateRoadPartLeft(GetMod(1), entryPoints);
				CreateRoadPartRight(GetMod(2), entryPoints);
				break;
			}
#undef GetMod
		}
		if(fillRoads)
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
				CreateRoad(Rect(0, h / 2 - roadPart - rs1, roadPart, h / 2 - roadPart + rs2), t);
				CreateEntry(entryPoints, ED_LeftBottom);
				CreateRoad(Rect(w - roadPart, h / 2 + roadPart - rs1, w - 1, h / 2 + roadPart + rs2), t);
				CreateEntry(entryPoints, ED_RightTop);
				pts[0] = Int2(roadPart, h / 2 - roadPart);
				pts[1] = Int2(w / 2, h / 2 - roadPart);
				pts[2] = Int2(w / 2, h / 2 + roadPart);
				pts[3] = Int2(w - roadPart, h / 2 + roadPart);
				break;
			case GDIR_RIGHT:
				CreateRoad(Rect(w - roadPart, h / 2 - roadPart - rs1, w - 1, h / 2 - roadPart + rs2), t);
				CreateEntry(entryPoints, ED_RightBottom);
				CreateRoad(Rect(0, h / 2 + roadPart - rs1, roadPart, h / 2 + roadPart + rs2), t);
				CreateEntry(entryPoints, ED_LeftTop);
				pts[0] = Int2(w - roadPart, h / 2 - roadPart);
				pts[2] = Int2(w / 2, h / 2 + roadPart);
				pts[1] = Int2(w / 2, h / 2 - roadPart);
				pts[3] = Int2(roadPart, h / 2 + roadPart);
				break;
			case GDIR_DOWN:
				CreateRoad(Rect(w / 2 - roadPart - rs1, 0, w / 2 - roadPart + rs2, roadPart), t);
				CreateEntry(entryPoints, ED_BottomLeft);
				CreateRoad(Rect(w / 2 + roadPart - rs1, h - roadPart, w / 2 + roadPart + rs2, h - 1), t);
				CreateEntry(entryPoints, ED_TopRight);
				pts[0] = Int2(w / 2 - roadPart, roadPart);
				pts[1] = Int2(w / 2 - roadPart, h / 2);
				pts[2] = Int2(w / 2 + roadPart, h / 2);
				pts[3] = Int2(w / 2 + roadPart, h - roadPart);
				break;
			case GDIR_UP:
				CreateRoad(Rect(w / 2 - roadPart - rs1, h - roadPart, w / 2 - roadPart + rs2, h - 1), t);
				CreateEntry(entryPoints, ED_TopLeft);
				CreateRoad(Rect(w / 2 + roadPart - rs1, 0, w / 2 + roadPart + rs2, roadPart), t);
				CreateEntry(entryPoints, ED_BottomRight);
				pts[0] = Int2(w / 2 - roadPart, h - roadPart);
				pts[1] = Int2(w / 2 - roadPart, h / 2);
				pts[2] = Int2(w / 2 + roadPart, h / 2);
				pts[3] = Int2(w / 2 + roadPart, roadPart);
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
				CreateRoadPartLeft(t, entryPoints);
				break;
			case GDIR_RIGHT:
				gates = GATE_EAST;
				CreateRoadPartRight(t, entryPoints);
				break;
			case GDIR_DOWN:
				gates = GATE_SOUTH;
				CreateRoadPartBottom(t, entryPoints);
				break;
			case GDIR_UP:
				gates = GATE_NORTH;
				CreateRoadPartTop(t, entryPoints);
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
				if(Vec3::DistanceSquared(PtToPos(pt), PtToPos(center)) <= Pow2(10.f))
					tiles[pt(w)].Set(rocky_roads > 0 ? TT_ROAD : TT_SAND, TT_GRASS, 0, TM_ROAD);
			}
		}
	}

	if(!roads.empty())
	{
		for(int i = 0; i < (int)roads.size(); ++i)
		{
			const Road& r = roads[i];
			int minx = max(0, r.start.x - 1),
				miny = max(0, r.start.y - 1),
				maxx = min(w - 1, r.end.x + 1),
				maxy = min(h - 1, r.end.y + 1);
			for(int y = miny; y <= maxy; ++y)
			{
				for(int x = minx; x <= maxx; ++x)
					roadIds[x + y * w] = i;
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
// nie obs�uguje kraw�dzi!
void CityGenerator::CreateCurveRoad(Int2 points[], uint count, TERRAIN_TILE t)
{
	assert(count == 3 || count == 4);

	if(count == 3)
		Pixel::PlotQuadBezier(points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y, 1.f, (float)roadSize, pixels);
	else
		Pixel::PlotCubicBezier(points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y, points[3].x, points[3].y, (float)roadSize, pixels);

	// zr�b co� z pixelami
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
		// gdy to by�o to pojawia� si� piach zamiast drogi :S
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
		Building& building = *build_it->building;
		Int2 ext = building.size - Int2(1, 1);

		bool ok;
		vector<BuildPt> points;
		int side;

		// side - w kt�r� stron� mo�e by� obr�cony budynek w tym miejscu
		// -1 - brak
		// 0 - po x
		// 1 - po z
		// 2 - po x i po z

		// miejsca na domy
		const int ymin = int(0.25f * w);
		const int ymax = int(0.75f * w);
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
			Error("Failed to generate city map! No place for building %s!", building.id.c_str());
			return;
		}

		// ustal pozycj� i obr�t budynku
		if(IsSet(building.flags, Building::FAVOR_DIST))
		{
			int bestTotal = 0;
			for(vector<BuildPt>::iterator it = points.begin(), end = points.end(); it != end; ++it)
			{
				const Int2 mod[] = {
					Int2(-1,-1),
					Int2(-1,0),
					Int2(-1,1),
					Int2(0,-1),
					Int2(0,1),
					Int2(1,-1),
					Int2(1,0),
					Int2(1,1)
				};
				int bestDist = 999;
				for(int i = 0; i < 8; ++i)
				{
					Int2 pt = it->pt;
					int dist = 0;
					while(true)
					{
						pt += mod[i];
						if(pt.x < ymin || pt.y < ymin || pt.x > ymax || pt.y > ymax)
							break;
						if(tiles[pt.x + pt.y * s].mode != TM_NORMAL)
						{
							if(dist < bestDist)
								bestDist = dist;
							break;
						}
						++dist;
					}
				}
				if(bestDist >= bestTotal)
				{
					if(bestDist > bestTotal)
					{
						validPts.clear();
						bestTotal = bestDist;
					}

					GameDirection dir;
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
					validPts.push_back(std::make_pair(it->pt, dir));
				}
			}
		}
		else
		{
			const Int2 centrum(w / 2, w / 2);
			int bestRange = INT_MAX;
			for(vector<BuildPt>::iterator it = points.begin(), end = points.end(); it != end; ++it)
			{
				int bestLength = 999;
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
						TerrainTile& t = tiles[pt.x + pt.y * w];
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

					if(tiles[pt.x + pt.y * w].mode == TM_ROAD && length < bestLength)
					{
						bestLength = length;
						dir = GDIR_DOWN;
					}

					// up
					length = 1;
					pt = it->pt;
					++pt.y;

					while(1)
					{
						TerrainTile& t = tiles[pt.x + pt.y * w];
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

					if(tiles[pt.x + pt.y * w].mode == TM_ROAD && length < bestLength)
					{
						bestLength = length;
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
						TerrainTile& t = tiles[pt.x + pt.y * w];
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

					if(tiles[pt.x + pt.y * w].mode == TM_ROAD && length < bestLength)
					{
						bestLength = length;
						dir = GDIR_LEFT;
					}

					// right
					length = 1;
					pt = it->pt;
					++pt.x;

					while(1)
					{
						TerrainTile& t = tiles[pt.x + pt.y * w];
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

					if(tiles[pt.x + pt.y * w].mode == TM_ROAD && length < bestLength)
					{
						bestLength = length;
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

				int range;
				if(IsSet(building.flags, Building::FAVOR_CENTER))
					range = Int2::Distance(centrum, it->pt);
				else
					range = 0;
				if(IsSet(building.flags, Building::FAVOR_ROAD))
					range += max(0, bestLength - 1);
				else
					range += max(0, bestLength - 5);

				if(range <= bestRange)
				{
					if(range < bestRange)
					{
						validPts.clear();
						bestRange = range;
					}
					validPts.push_back(std::make_pair(it->pt, dir));
				}
			}
		}

		pair<Int2, GameDirection> pt = RandomItem(validPts);
		GameDirection best_dir = pt.second;
		validPts.clear();

		// 0 - obr�cony w g�re
		// w:4 h:2
		// 8765
		// 4321
		// kolejno�� po x i y odwr�cona

		// 1 - obr�cony w lewo
		// 51
		// 62
		// 73
		// 84
		// x = y, x odwr�cone

		// 2 - obr�cony w d�
		// w:4 h:2
		// 1234
		// 5678
		// domy�lnie, nic nie zmieniaj

		// 3 - obr�cony w prawo
		// 48
		// 37
		// 26
		// 15
		// x = y, x i y odwr�cone

		build_it->pt = pt.first;
		build_it->dir = best_dir;

		Int2 ext2 = building.size;
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
					scheme = building.scheme[xr + (ext2.y - yr - 1) * ext2.x];
					break;
				case GDIR_LEFT:
					scheme = building.scheme[ext2.y - yr - 1 + (ext2.x - xr - 1) * ext2.y];
					break;
				case GDIR_UP:
					scheme = building.scheme[ext2.x - xr - 1 + yr * ext2.x];
					break;
				case GDIR_RIGHT:
					scheme = building.scheme[yr + xr * ext2.y];
					break;
				default:
					assert(0);
					break;
				}

				Int2 pt2(pt.first.x + xx, pt.first.y + yy);

				TerrainTile& t = tiles[pt2.x + (pt2.y) * w];
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
					build_it->unitPt = pt2;
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
				sum += height[pt2.x + pt2.y * (w + 1)];
				tmpPts.push_back(pt2);
			}
		}

		// set height
		sum /= count;
		for(Int2& pt : tmpPts)
			height[pt.x + pt.y * (w + 1)] = sum;
		tmpPts.clear();

		// generate path
		if(!IsSet(building.flags, Building::NO_PATH))
		{
			assert(road_start != Int2(-1, -1));
			if(road_start != Int2(-1, -1))
				GeneratePath(road_start);
		}
	}
}

//=================================================================================================
void CityGenerator::GeneratePath(const Int2& pt)
{
	assert(pt.x >= 0 && pt.y >= 0 && pt.x < w && pt.y < h);

	int size = w * h;
	if(size > (int)grid.size())
		grid.resize(size);
	memset(&grid[0], 0, sizeof(APoint2) * size);

	int start_idx = pt.x + pt.y * w;
	toCheck.push_back(start_idx);

	grid[start_idx].state = 1;
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
	const int x_min = int(float(w) * 0.2f);
	const int x_max = int(float(w) * 0.8f);
	const int y_min = int(float(h) * 0.2f);
	const int y_max = int(float(h) * 0.8f);

	while(!toCheck.empty())
	{
		int pt_idx = toCheck.back();
		Int2 pt(pt_idx % w, pt_idx / w);
		toCheck.pop_back();
		if(pt.x <= x_min || pt.x >= x_max || pt.y <= y_min || pt.y >= y_max)
			continue;

		APoint2& this_point = grid[pt_idx];

		for(int i = 0; i < 8; ++i)
		{
			const int idx = pt_idx + mod[i].change.x + mod[i].change.y * w;
			APoint2& point = grid[idx];
			if(point.state == 0)
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
					point.state = 1;
					point.cost = this_point.cost + (this_point.dir == i ? mod[i].cost : mod[i].cost2);
					point.dir = i;
					toCheck.push_back(idx);
				}
				else
					point.state = 1;
			}
		}

		std::sort(toCheck.begin(), toCheck.end(), sorter);
	}

superbreak:
	toCheck.clear();

	assert(end_tile_idx != -1);

	Int2 pt2(end_tile_idx % w, end_tile_idx / w);
	bool go = true;
	while(go)
	{
		TerrainTile& t = tiles[pt2.x + pt2.y * w];
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
		const APoint2& apt = grid[pt2.x + pt2.y * w];
		if(apt.dir > 3)
		{
			const Mod& m = mod[apt.dir];
			{
				TerrainTile& t = tiles[pt2.x + m.back.x + pt2.y * w];
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
				TerrainTile& t = tiles[pt2.x + (pt2.y + m.back.y) * w];
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
			if(Any(tiles[x + y * w].mode, TM_ROAD, TM_PATH))
			{
				for(int i = 1; i < 21; ++i)
					block[i] = !tiles[x + blocked[i].x + (y + blocked[i].y) * w].IsBuilding();

				float sum = 0.f;
				for(int i = 0; i < 12; ++i)
				{
					const Nei& nei = neis[i];
					Int2 pt(x + nei.pt.x, y + nei.pt.y);
					sum += height[pt(w1)];
					if(block[nei.id[0]] && block[nei.id[1]] && block[nei.id[2]] && block[nei.id[3]])
						tmpPts.push_back(pt);
				}

				sum /= 12;
				for(Int2& pt : tmpPts)
					height[pt(w1)] = sum;
				tmpPts.clear();
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
			if(tiles[x + y * w].mode < TM_BUILDING_SAND
				&& tiles[x - 1 + y * w].mode < TM_BUILDING_SAND
				&& tiles[x + (y - 1) * w].mode < TM_BUILDING_SAND
				&& tiles[x - 1 + (y - 1) * w].mode < TM_BUILDING_SAND)
			{
				float avg = (height[x + y * (w + 1)]
					+ height[x - 1 + y * (w + 1)]
					+ height[x + 1 + y * (w + 1)]
					+ height[x + (y - 1) * (h + 1)]
					+ height[x + (y + 1) * (h + 1)]) / 5;
				height[x + y * (w + 1)] = avg;
			}
		}
	}
}

//=================================================================================================
void CityGenerator::CreateRoadLineLeftRight(TERRAIN_TILE t, vector<EntryPoint>& entryPoints)
{
	CreateRoad(Rect(0, h / 2 - rs1, w - 1, h / 2 + rs2), t);
	CreateEntry(entryPoints, ED_Left);
	CreateEntry(entryPoints, ED_Right);
}

//=================================================================================================
void CityGenerator::CreateRoadLineBottomTop(TERRAIN_TILE t, vector<EntryPoint>& entryPoints)
{
	CreateRoad(Rect(w / 2 - rs1, 0, w / 2 + rs2, h - 1), t);
	CreateEntry(entryPoints, ED_Bottom);
	CreateEntry(entryPoints, ED_Top);
}

//=================================================================================================
void CityGenerator::CreateRoadPartLeft(TERRAIN_TILE t, vector<EntryPoint>& entryPoints)
{
	CreateRoad(Rect(0, h / 2 - rs1, w / 2 - rs1 - 1, h / 2 + rs2), t);
	CreateEntry(entryPoints, ED_Left);
}

//=================================================================================================
void CityGenerator::CreateRoadPartRight(TERRAIN_TILE t, vector<EntryPoint>& entryPoints)
{
	CreateRoad(Rect(w / 2 + rs2 + 1, h / 2 - rs1, w - 1, h / 2 + rs2), t);
	CreateEntry(entryPoints, ED_Right);
}

//=================================================================================================
void CityGenerator::CreateRoadPartBottom(TERRAIN_TILE t, vector<EntryPoint>& entryPoints)
{
	CreateRoad(Rect(w / 2 - rs1, 0, w / 2 + rs1, h / 2 - rs1 - 1), t);
	CreateEntry(entryPoints, ED_Bottom);
}

//=================================================================================================
void CityGenerator::CreateRoadPartTop(TERRAIN_TILE t, vector<EntryPoint>& entryPoints)
{
	CreateRoad(Rect(w / 2 - rs1, h / 2 + rs2 + 1, w / 2 + rs2, h - 1), t);
	CreateEntry(entryPoints, ED_Top);
}

//=================================================================================================
void CityGenerator::CreateRoadCenter(TERRAIN_TILE t)
{
	CreateRoad(Rect(w / 2 - rs1, h / 2 - rs1, w / 2 + rs2, h / 2 + rs2), t);
}

//=================================================================================================
void CityGenerator::CreateEntry(vector<EntryPoint>& entryPoints, EntryDir dir)
{
	EntryPoint& ep = Add1(entryPoints);

	switch(dir)
	{
	case ED_Left:
		{
			Vec2 p(SPAWN_RATIO * w * 2, float(h) + 1);
			ep.spawnRot = PI * 3 / 2;
			ep.spawnRegion = Box2d(p.x - SPAWN_RANGE, p.y - SPAWN_RANGE, p.x + SPAWN_RANGE, p.y + SPAWN_RANGE);
			ep.exitRegion = Box2d(p.x - EXIT_END, p.y - EXIT_WIDTH, p.x - EXIT_START, p.y + EXIT_WIDTH);
		}
		break;
	case ED_Right:
		{
			Vec2 p((1.f - SPAWN_RATIO) * w * 2, float(h) + 1);
			ep.spawnRot = PI / 2;
			ep.spawnRegion = Box2d(p.x - SPAWN_RANGE, p.y - SPAWN_RANGE, p.x + SPAWN_RANGE, p.y + SPAWN_RANGE);
			ep.exitRegion = Box2d(p.x + EXIT_START, p.y - EXIT_WIDTH, p.x + EXIT_END, p.y + EXIT_WIDTH);
		}
		break;
	case ED_Bottom:
		{
			Vec2 p(float(w) + 1, SPAWN_RATIO * h * 2);
			ep.spawnRot = PI;
			ep.spawnRegion = Box2d(p.x - SPAWN_RANGE, p.y - SPAWN_RANGE, p.x + SPAWN_RANGE, p.y + SPAWN_RANGE);
			ep.exitRegion = Box2d(p.x - EXIT_WIDTH, p.y - EXIT_END, p.x + EXIT_WIDTH, p.y - EXIT_START);
		}
		break;
	case ED_Top:
		{
			Vec2 p(float(w) + 1, (1.f - SPAWN_RATIO) * h * 2);
			ep.spawnRot = 0;
			ep.spawnRegion = Box2d(p.x - SPAWN_RANGE, p.y - SPAWN_RANGE, p.x + SPAWN_RANGE, p.y + SPAWN_RANGE);
			ep.exitRegion = Box2d(p.x - EXIT_WIDTH, p.y + EXIT_START, p.x + EXIT_WIDTH, p.y + EXIT_END);
		}
		break;
	case ED_LeftBottom:
		{
			Vec2 p(SPAWN_RATIO * w * 2, float(h / 2 - roadPart) * 2 + 1);
			ep.spawnRot = PI * 3 / 2;
			ep.spawnRegion = Box2d(p.x - SPAWN_RANGE, p.y - SPAWN_RANGE, p.x + SPAWN_RANGE, p.y + SPAWN_RANGE);
			ep.exitRegion = Box2d(p.x - EXIT_END, p.y - EXIT_WIDTH, p.x + EXIT_START, p.y + EXIT_WIDTH);
		}
		break;
	case ED_LeftTop:
		{
			Vec2 p(SPAWN_RATIO * w * 2, float(h / 2 + roadPart) * 2 + 1);
			ep.spawnRot = PI * 3 / 2;
			ep.spawnRegion = Box2d(p.x - SPAWN_RANGE, p.y - SPAWN_RANGE, p.x + SPAWN_RANGE, p.y + SPAWN_RANGE);
			ep.exitRegion = Box2d(p.x - EXIT_END, p.y - EXIT_WIDTH, p.x + EXIT_START, p.y + EXIT_WIDTH);
		}
		break;
	case ED_RightBottom:
		{
			Vec2 p((1.f - SPAWN_RATIO) * w * 2, float(h / 2 - roadPart) * 2 + 1);
			ep.spawnRot = PI / 2;
			ep.spawnRegion = Box2d(p.x - SPAWN_RANGE, p.y - SPAWN_RANGE, p.x + SPAWN_RANGE, p.y + SPAWN_RANGE);
			ep.exitRegion = Box2d(p.x + EXIT_START, p.y - EXIT_WIDTH, p.x + EXIT_END, p.y + EXIT_WIDTH);
		}
		break;
	case ED_RightTop:
		{
			Vec2 p((1.f - SPAWN_RATIO) * w * 2, float(h / 2 + roadPart) * 2 + 1);
			ep.spawnRot = PI / 2;
			ep.spawnRegion = Box2d(p.x - SPAWN_RANGE, p.y - SPAWN_RANGE, p.x + SPAWN_RANGE, p.y + SPAWN_RANGE);
			ep.exitRegion = Box2d(p.x + EXIT_START, p.y - EXIT_WIDTH, p.x + EXIT_END, p.y + EXIT_WIDTH);
		}
		break;
	case ED_BottomLeft:
		{
			Vec2 p(float(w / 2 - roadPart) * 2 + 1, SPAWN_RATIO * h * 2);
			ep.spawnRot = PI;
			ep.spawnRegion = Box2d(p.x - SPAWN_RANGE, p.y - SPAWN_RANGE, p.x + SPAWN_RANGE, p.y + SPAWN_RANGE);
			ep.exitRegion = Box2d(p.x - EXIT_WIDTH, p.y - EXIT_END, p.x + EXIT_WIDTH, p.y + EXIT_START);
		}
		break;
	case ED_BottomRight:
		{
			Vec2 p(float(w / 2 + roadPart) * 2 + 1, SPAWN_RATIO * h * 2);
			ep.spawnRot = PI;
			ep.spawnRegion = Box2d(p.x - SPAWN_RANGE, p.y - SPAWN_RANGE, p.x + SPAWN_RANGE, p.y + SPAWN_RANGE);
			ep.exitRegion = Box2d(p.x - EXIT_WIDTH, p.y - EXIT_END, p.x + EXIT_WIDTH, p.y + EXIT_START);
		}
		break;
	case ED_TopLeft:
		{
			Vec2 p(float(w / 2 - roadPart) * 2 + 1, (1.f - SPAWN_RATIO) * h * 2);
			ep.spawnRot = 0;
			ep.spawnRegion = Box2d(p.x - SPAWN_RANGE, p.y - SPAWN_RANGE, p.x + SPAWN_RANGE, p.y + SPAWN_RANGE);
			ep.exitRegion = Box2d(p.x - EXIT_WIDTH, p.y + EXIT_START, p.x + EXIT_WIDTH, p.y + EXIT_END);
		}
		break;
	case ED_TopRight:
		{
			Vec2 p(float(w / 2 + roadPart) * 2 + 1, (1.f - SPAWN_RATIO) * h * 2);
			ep.spawnRot = 0;
			ep.spawnRegion = Box2d(p.x - SPAWN_RANGE, p.y - SPAWN_RANGE, p.x + SPAWN_RANGE, p.y + SPAWN_RANGE);
			ep.exitRegion = Box2d(p.x - EXIT_WIDTH, p.y + EXIT_START, p.x + EXIT_WIDTH, p.y + EXIT_END);
		}
		break;
	}

	ep.exitY = 0;
}

//=================================================================================================
void CityGenerator::CleanBorders()
{
	// bottom / top
	for(int x = 1; x < w; ++x)
	{
		height[x] = height[x + (w + 1)];
		height[x + h * (w + 1)] = height[x + (h - 1) * (w + 1)];
	}

	// left / right
	for(int y = 1; y < h; ++y)
	{
		height[y * (w + 1)] = height[y * (w + 1) + 1];
		height[(y + 1) * (w + 1) - 1] = height[(y + 1) * (w + 1) - 2];
	}

	// corners
	height[0] = (height[1] + height[w + 1]) / 2;
	height[w] = (height[w - 1] + height[(w + 1) * 2 - 1]) / 2;
	height[h * (w + 1)] = (height[1 + h * (w + 1)] + height[(h - 1) * (w + 1)]) / 2;
	height[(h + 1) * (w + 1) - 1] = (height[(h + 1) * (w + 1) - 2] + height[h * (w + 1) - 1]) / 2;
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
		if(tiles[xx + (h - 15) * w].mode == TM_ROAD)
		{
			float th = height[xx + (h - 15) * (w + 1)];
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
	const int ymin = int(0.25f * w);
	const int ymax = int(0.75f * w) - 5;
	fields.clear();

	for(int i = 0; i < 50; ++i)
	{
		Int2 pt(Random(ymin, ymax), Random(ymin, ymax));
		if(tiles[pt.x + pt.y * w].mode != TM_NORMAL)
			continue;

		Int2 size(Random(4, 8), Random(4, 8));
		if(size.x > size.y)
			size.x *= 2;
		else
			size.y *= 2;

		for(int y = pt.y - 1; y <= pt.y + size.y; ++y)
		{
			for(int x = pt.x - 1; x <= pt.x + size.x; ++x)
			{
				if(tiles[x + y * w].mode != TM_NORMAL)
					goto next;
			}
		}

		for(int y = pt.y; y < pt.y + size.y; ++y)
		{
			for(int x = pt.x; x < pt.x + size.x; ++x)
			{
				tiles[x + y * w].Set(TT_FIELD, TM_FIELD);
				float avg = (height[x + y * (w + 1)]
					+ height[x + y * (w + 1)]
					+ height[x + (y + 1) * (w + 1)]
					+ height[x + 1 + (y - 1) * (w + 1)]
					+ height[x + 1 + (y + 1) * (w + 1)]) / 5;
				height[x + y * (w + 1)] = avg;
				height[x + (y + 1) * (w + 1)] = avg;
				height[x + 1 + (y - 1) * (w + 1)] = avg;
				height[x + 1 + (y + 1) * (w + 1)] = avg;
			}
		}

		fields.push_back(Rect::Create(pt, size));

	next:;
	}
}

//=================================================================================================
void CityGenerator::ApplyWallTiles(int gates)
{
	const int mur1 = int(0.15f * w);
	const int mur2 = int(0.85f * w);
	const int w1 = w + 1;

	// tiles under walls
	for(int i = mur1; i <= mur2; ++i)
	{
		// north
		tiles[i + mur1 * w].Set(TT_SAND, TM_BUILDING);
		if(tiles[i + (mur1 + 1) * w].t == TT_GRASS)
			tiles[i + (mur1 + 1) * w].Set(TT_SAND, TT_GRASS, 128, TM_BUILDING);
		height[i + (mur1 - 2) * w1] = 1.f;
		height[i + (mur1 - 1) * w1] = 1.f;
		height[i + mur1 * w1] = 1.f;
		height[i + (mur1 + 1) * w1] = 1.f;
		height[i + (mur1 + 2) * w1] = 1.f;
		// south
		tiles[i + mur2 * w].Set(TT_SAND, TM_BUILDING);
		if(tiles[i + (mur2 - 1) * w].t == TT_GRASS)
			tiles[i + (mur2 - 1) * w].Set(TT_SAND, TT_GRASS, 128, TM_BUILDING);
		height[i + (mur2 + 2) * w1] = 1.f;
		height[i + (mur2 + 1) * w1] = 1.f;
		height[i + mur2 * w1] = 1.f;
		height[i + (mur2 - 1) * w1] = 1.f;
		height[i + (mur2 - 2) * w1] = 1.f;
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
		tiles[w / 2 - 1 + int(0.15f * w) * w].Set(TT_ROAD, TM_ROAD);
		tiles[w / 2 + int(0.15f * w) * w].Set(TT_ROAD, TM_ROAD);
		tiles[w / 2 + 1 + int(0.15f * w) * w].Set(TT_ROAD, TM_ROAD);
		tiles[w / 2 - 1 + (int(0.15f * w) + 1) * w].Set(TT_ROAD, TM_ROAD);
		tiles[w / 2 + (int(0.15f * w) + 1) * w].Set(TT_ROAD, TM_ROAD);
		tiles[w / 2 + 1 + (int(0.15f * w) + 1) * w].Set(TT_ROAD, TM_ROAD);
	}
	if(IsSet(gates, GATE_WEST))
	{
		tiles[int(0.15f * w) + (w / 2 - 1) * w].Set(TT_ROAD, TM_ROAD);
		tiles[int(0.15f * w) + (w / 2) * w].Set(TT_ROAD, TM_ROAD);
		tiles[int(0.15f * w) + (w / 2 + 1) * w].Set(TT_ROAD, TM_ROAD);
		tiles[int(0.15f * w) + 1 + (w / 2 - 1) * w].Set(TT_ROAD, TM_ROAD);
		tiles[int(0.15f * w) + 1 + (w / 2) * w].Set(TT_ROAD, TM_ROAD);
		tiles[int(0.15f * w) + 1 + (w / 2 + 1) * w].Set(TT_ROAD, TM_ROAD);
	}
	if(IsSet(gates, GATE_NORTH))
	{
		tiles[w / 2 - 1 + int(0.85f * w) * w].Set(TT_ROAD, TM_ROAD);
		tiles[w / 2 + int(0.85f * w) * w].Set(TT_ROAD, TM_ROAD);
		tiles[w / 2 + 1 + int(0.85f * w) * w].Set(TT_ROAD, TM_ROAD);
		tiles[w / 2 - 1 + (int(0.85f * w) - 1) * w].Set(TT_ROAD, TM_ROAD);
		tiles[w / 2 + (int(0.85f * w) - 1) * w].Set(TT_ROAD, TM_ROAD);
		tiles[w / 2 + 1 + (int(0.85f * w) - 1) * w].Set(TT_ROAD, TM_ROAD);
	}
	if(IsSet(gates, GATE_EAST))
	{
		tiles[int(0.85f * w) + (w / 2 - 1) * w].Set(TT_ROAD, TM_ROAD);
		tiles[int(0.85f * w) + (w / 2) * w].Set(TT_ROAD, TM_ROAD);
		tiles[int(0.85f * w) + (w / 2 + 1) * w].Set(TT_ROAD, TM_ROAD);
		tiles[int(0.85f * w) - 1 + (w / 2 - 1) * w].Set(TT_ROAD, TM_ROAD);
		tiles[int(0.85f * w) - 1 + (w / 2) * w].Set(TT_ROAD, TM_ROAD);
		tiles[int(0.85f * w) - 1 + (w / 2 + 1) * w].Set(TT_ROAD, TM_ROAD);
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
	enum RoadPart
	{
		RP_START,
		RP_END,
		RP_MID
	};

	roadTile = _road_tile;
	toCheck.clear();
	for(int i = 0; i < (int)roads.size(); ++i)
		toCheck.push_back(i);

	const int ROAD_MIN_X = (int)(CITY_BORDER_MIN * w),
		ROAD_MAX_X = (int)(CITY_BORDER_MAX * w),
		ROAD_MIN_Y = (int)(CITY_BORDER_MIN * h),
		ROAD_MAX_Y = (int)(CITY_BORDER_MAX * h);
	int choice[3];

	for(int i = 0; i < tries; ++i)
	{
		if(toCheck.empty())
			break;

		int index = RandomItemPop(toCheck);
		Road& r = roads[index];

		int choices = 0;
		if(!IsSet(r.flags, ROAD_START_CHECKED))
			choice[choices++] = RP_START;
		if(!IsSet(r.flags, ROAD_MID_CHECKED))
			choice[choices++] = RP_MID;
		if(!IsSet(r.flags, ROAD_END_CHECKED))
			choice[choices++] = RP_END;
		if(choices == 0)
			continue;

		const int type = choice[Rand() % choices];

		Int2 pt;
		const bool horizontal = IsSet(r.flags, ROAD_HORIZONTAL);

		if(type == RP_MID)
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
			if(type == RP_START)
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
				choice[2] = (type == RP_START ? GDIR_LEFT : GDIR_RIGHT);
			}
			else
			{
				choice[0] = GDIR_LEFT;
				choice[1] = GDIR_RIGHT;
				choice[2] = (type == RP_END ? GDIR_DOWN : GDIR_UP);
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
			toCheck.push_back(index);
	}

	toCheck.clear();
}

//=================================================================================================
int CityGenerator::MakeRoad(const Int2& start_pt, GameDirection dir, int roadIndex, int& collidedRoad)
{
	collidedRoad = -1;

	Int2 pt = start_pt, prev_pt;
	bool horizontal = (dir == GDIR_LEFT || dir == GDIR_RIGHT);
	int dist = 0,
		minx = int(CITY_BORDER_MIN * w),
		miny = int(CITY_BORDER_MIN * h),
		maxx = int(CITY_BORDER_MAX * w),
		maxy = int(CITY_BORDER_MAX * h);
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
			int j = pt.x + i * imod.x + (pt.y + i * imod.y) * w;
			int road_index2 = roadIds[j];
			if(tiles[j].mode != TM_NORMAL && road_index2 != roadIndex)
			{
				collidedRoad = road_index2;
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
	Road& road = Add1(roads);
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
				tiles[j].Set(roadTile, TM_ROAD);
			int& road_id = roadIds[j];
			if(road_id == -1)
				road_id = index;
		}
	}

	if(road.Length() < ROAD_MIN_MID_SPLIT)
		road.flags |= ROAD_MID_CHECKED;

	toCheck.push_back(index);
}

//=================================================================================================
bool CityGenerator::MakeAndFillRoad(const Int2& pt, GameDirection dir, int roadIndex)
{
	int collidedRoad;
	int road_dist = MakeRoad(pt, dir, roadIndex, collidedRoad);
	if(collidedRoad != -1)
	{
		if(Rand() % ROAD_JOIN_CHANCE == 0)
			++road_dist;
		else
		{
			road_dist -= ROAD_CHECK;
			collidedRoad = -1;
		}
	}
	if(road_dist >= ROAD_MIN_DIST)
	{
		if(collidedRoad == -1)
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
	TerrainTile* tiles = new TerrainTile[s * s];
	float* h = new float[(s + 1) * (s + 1)];
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
		if(tiles[x - 1 + (y - 1) * w].mode == TM_ROAD)
			return true;
	}

	if(x != w && y > 0)
	{
		if(tiles[x + (y - 1) * w].mode == TM_ROAD)
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
	else
	{
		steps += 2; // txGeneratingUnits, txGeneratingPhysics
		if(loc->lastVisit != world->GetWorldtime())
			++steps; // txGeneratingItems
	}
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
	const float hmax = village ? Random(4.f, 10.f) : Random(2.5f, 5.f);
	const int octaves = Random(2, 8);
	const float frequency = Random(3.f, 16.f);
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

		GenerateMainRoad(rtype, dir, roads, plaza, swap, city->entryPoints, city->gates, extra_roads);
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

		haveWell = false;
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

		GenerateMainRoad(rtype, dir, 4, plaza, swap, city->entryPoints, city->gates, true);
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
			haveWell = true;
			wellPt = Int2(64, 64);
		}
		else
			haveWell = false;
	}

	// budynki
	city->buildings.resize(tobuild.size());
	vector<ToBuild>::iterator build_it = tobuild.begin();
	for(vector<CityBuilding>::iterator it = city->buildings.begin(), end = city->buildings.end(); it != end; ++it, ++build_it)
	{
		it->building = build_it->building;
		it->pt = build_it->pt;
		it->dir = build_it->dir;
		it->unitPt = build_it->unitPt;
	}

	if(!village)
	{
		// set exits y
		terrain->SetHeightMap(city->h);
		for(vector<EntryPoint>::iterator entry_it = city->entryPoints.begin(), entry_end = city->entryPoints.end(); entry_it != entry_end; ++entry_it)
			entry_it->exitY = terrain->GetH(entry_it->exitRegion.Midpoint()) + 0.1f;
		terrain->RemoveHeightMap();
	}

	CleanBorders();
}

//=================================================================================================
void CityGenerator::OnEnter()
{
	game->arena->free = true;

	gameLevel->Apply();
	ApplyTiles();

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
	else
	{
		// remove temporary/quest units
		if(city->reset)
			RemoveTemporaryUnits();

		// remove blood/corpses
		int days;
		city->CheckUpdate(days, world->GetWorldtime());
		if(days > 0)
			gameLevel->UpdateLocation(days, 100, false);

		// recreate units
		game->LoadingStep(game->txGeneratingUnits);
		RespawnUnits();
		RepositionUnits();

		// recreate physics
		game->LoadingStep(game->txGeneratingPhysics);
		gameLevel->RecreateObjects();
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

		gameLevel->OnRevisitLevel();
	}

	SetOutsideParams();
	SetBuildingsParams();

	// create colliders
	game->LoadingStep(game->txRecreatingObjects);
	gameLevel->SpawnTerrainCollider();
	SpawnCityPhysics();
	SpawnOutsideBariers();
	for(InsideBuilding* b : city->insideBuildings)
	{
		b->mine = Int2(b->levelShift.x * 256, b->levelShift.y * 256);
		b->maxe = b->mine + Int2(256, 256);
	}

	// spawn quest units
	if(gameLevel->location->activeQuest && gameLevel->location->activeQuest != ACTIVE_QUEST_HOLDER)
	{
		Quest_Dungeon* quest = dynamic_cast<Quest_Dungeon*>(gameLevel->location->activeQuest);
		if(quest && !quest->done)
			questMgr->HandleQuestEvent(quest);
	}

	// generate minimap
	game->LoadingStep(game->txGeneratingMinimap);
	CreateMinimap();

	// add player team
	Vec3 spawn_pos;
	float spawn_dir;
	city->GetEntry(spawn_pos, spawn_dir);
	gameLevel->AddPlayerTeam(spawn_pos, spawn_dir);

	questMgr->GenerateQuestUnits(true);

	for(Unit& unit : team->members)
	{
		if(unit.IsHero())
			unit.hero->lostPvp = false;
	}

	team->CheckTeamItemShares();

	Quest_Contest* contest = questMgr->questContest;
	if(!contest->generated && gameLevel->locationIndex == contest->where && contest->state == Quest_Contest::CONTEST_TODAY)
		contest->SpawnDrunkmans();
}

//=================================================================================================
void CityGenerator::SpawnBuildings()
{
	LocationPart& locPart = *city;
	const int mur1 = int(0.15f * OutsideLocation::size);
	const int mur2 = int(0.85f * OutsideLocation::size);

	// building objects
	for(vector<CityBuilding>::iterator it = city->buildings.begin(), end = city->buildings.end(); it != end; ++it)
	{
		Object* o = new Object;
		o->pos = Vec3(float(it->pt.x + it->building->shift[it->dir].x) * 2, 1.f, float(it->pt.y + it->building->shift[it->dir].y) * 2);
		terrain->SetY(o->pos);
		o->rot = Vec3(0, DirToRot(it->dir), 0);
		o->scale = 1.f;
		o->base = nullptr;
		o->mesh = it->building->mesh;
		locPart.objects.push_back(o);
	}

	// create walls, towers & gates
	if(!city->IsVillage())
	{
		const int mid = int(0.5f * OutsideLocation::size);
		BaseObject* oWall = BaseObject::Get("wall"),
			*oTower = BaseObject::Get("tower"),
			*oGate = BaseObject::Get("gate"),
			*oGrate = BaseObject::Get("grate");

		// walls
		for(int i = mur1; i < mur2; i += 3)
		{
			// north
			if(!IsSet(city->gates, GATE_NORTH) || i < mid - 1 || i > mid)
				gameLevel->SpawnObjectEntity(locPart, oWall, Vec3(float(i) * 2 + 1.f, 1.f, int(0.85f * OutsideLocation::size) * 2 + 1.f), 0);

			// south
			if(!IsSet(city->gates, GATE_SOUTH) || i < mid - 1 || i > mid)
				gameLevel->SpawnObjectEntity(locPart, oWall, Vec3(float(i) * 2 + 1.f, 1.f, int(0.15f * OutsideLocation::size) * 2 + 1.f), PI);

			// west
			if(!IsSet(city->gates, GATE_WEST) || i < mid - 1 || i > mid)
				gameLevel->SpawnObjectEntity(locPart, oWall, Vec3(int(0.15f * OutsideLocation::size) * 2 + 1.f, 1.f, float(i) * 2 + 1.f), PI * 3 / 2);

			// east
			if(!IsSet(city->gates, GATE_EAST) || i < mid - 1 || i > mid)
				gameLevel->SpawnObjectEntity(locPart, oWall, Vec3(int(0.85f * OutsideLocation::size) * 2 + 1.f, 1.f, float(i) * 2 + 1.f), PI / 2);
		}

		// towers
		// north east
		gameLevel->SpawnObjectEntity(locPart, oTower, Vec3(int(0.85f * OutsideLocation::size) * 2 + 1.f, 1.f, int(0.85f * OutsideLocation::size) * 2 + 1.f), 0);
		// south east
		gameLevel->SpawnObjectEntity(locPart, oTower, Vec3(int(0.85f * OutsideLocation::size) * 2 + 1.f, 1.f, int(0.15f * OutsideLocation::size) * 2 + 1.f), PI / 2);
		// south west
		gameLevel->SpawnObjectEntity(locPart, oTower, Vec3(int(0.15f * OutsideLocation::size) * 2 + 1.f, 1.f, int(0.15f * OutsideLocation::size) * 2 + 1.f), PI);
		// north west
		gameLevel->SpawnObjectEntity(locPart, oTower, Vec3(int(0.15f * OutsideLocation::size) * 2 + 1.f, 1.f, int(0.85f * OutsideLocation::size) * 2 + 1.f), PI * 3 / 2);

		// gates
		if(IsSet(city->gates, GATE_NORTH))
		{
			gameLevel->SpawnObjectEntity(locPart, oGate, Vec3(0.5f * OutsideLocation::size * 2 + 1.f, 1.f, 0.85f * OutsideLocation::size * 2), 0);
			gameLevel->SpawnObjectEntity(locPart, oGrate, Vec3(0.5f * OutsideLocation::size * 2 + 1.f, 1.f, 0.85f * OutsideLocation::size * 2), 0);
		}

		if(IsSet(city->gates, GATE_SOUTH))
		{
			gameLevel->SpawnObjectEntity(locPart, oGate, Vec3(0.5f * OutsideLocation::size * 2 + 1.f, 1.f, 0.15f * OutsideLocation::size * 2), PI);
			gameLevel->SpawnObjectEntity(locPart, oGrate, Vec3(0.5f * OutsideLocation::size * 2 + 1.f, 1.f, 0.15f * OutsideLocation::size * 2), PI);
		}

		if(IsSet(city->gates, GATE_WEST))
		{
			gameLevel->SpawnObjectEntity(locPart, oGate, Vec3(0.15f * OutsideLocation::size * 2, 1.f, 0.5f * OutsideLocation::size * 2 + 1.f), PI * 3 / 2);
			gameLevel->SpawnObjectEntity(locPart, oGrate, Vec3(0.15f * OutsideLocation::size * 2, 1.f, 0.5f * OutsideLocation::size * 2 + 1.f), PI * 3 / 2);
		}

		if(IsSet(city->gates, GATE_EAST))
		{
			gameLevel->SpawnObjectEntity(locPart, oGate, Vec3(0.85f * OutsideLocation::size * 2, 1.f, 0.5f * OutsideLocation::size * 2 + 1.f), PI / 2);
			gameLevel->SpawnObjectEntity(locPart, oGrate, Vec3(0.85f * OutsideLocation::size * 2, 1.f, 0.5f * OutsideLocation::size * 2 + 1.f), PI / 2);
		}
	}

	// building physics & subobjects
	for(vector<CityBuilding>::iterator it = city->buildings.begin(), end = city->buildings.end(); it != end; ++it)
	{
		Building* b = it->building;
		gameLevel->ProcessBuildingObjects(locPart, city, nullptr, b->mesh, b->insideMesh, DirToRot(it->dir), it->dir,
			Vec3(float(it->pt.x + b->shift[it->dir].x) * 2, 0.f, float(it->pt.y + b->shift[it->dir].y) * 2), b, &*it);
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
	LocationPart& locPart = *city;

	if(!outside_objects[0].obj)
	{
		for(uint i = 0; i < n_outside_objects; ++i)
			outside_objects[i].obj = BaseObject::Get(outside_objects[i].name);
	}

	// well
	if(haveWell)
	{
		Vec3 pos = PtToPos(wellPt);
		terrain->SetY(pos);
		gameLevel->SpawnObjectEntity(locPart, BaseObject::Get("coveredwell"), pos, PI / 2 * (Rand() % 4), 1.f, 0, nullptr);
	}

	// fields scarecrow
	if(city->IsVillage())
	{
		BaseObject* scarecrow = BaseObject::Get("scarecrow");
		for(const Rect& rect : fields)
		{
			if(Rand() % 3 != 0)
				continue;
			const Int2 size = rect.Size();
			Box2d spawnBox(2.f * (rect.p1.x + size.x / 4), 2.f * (rect.p1.y + size.y / 4),
				2.f * (rect.p2.x - size.x / 4), 2.f * (rect.p2.y - size.y / 4));
			Vec3 pos = spawnBox.GetRandomPos3();
			terrain->SetY(pos);
			gameLevel->SpawnObjectEntity(locPart, scarecrow, pos, Random(MAX_ANGLE));
		}
	}

	TerrainTile* tiles = city->tiles;

	for(int i = 0; i < 512; ++i)
	{
		Int2 pt(Random(1, OutsideLocation::size - 2), Random(1, OutsideLocation::size - 2));
		if(tiles[pt.x + pt.y * OutsideLocation::size].mode == TM_NORMAL
			&& tiles[pt.x - 1 + pt.y * OutsideLocation::size].mode == TM_NORMAL
			&& tiles[pt.x + 1 + pt.y * OutsideLocation::size].mode == TM_NORMAL
			&& tiles[pt.x + (pt.y - 1) * OutsideLocation::size].mode == TM_NORMAL
			&& tiles[pt.x + (pt.y + 1) * OutsideLocation::size].mode == TM_NORMAL
			&& tiles[pt.x - 1 + (pt.y - 1) * OutsideLocation::size].mode == TM_NORMAL
			&& tiles[pt.x - 1 + (pt.y + 1) * OutsideLocation::size].mode == TM_NORMAL
			&& tiles[pt.x + 1 + (pt.y - 1) * OutsideLocation::size].mode == TM_NORMAL
			&& tiles[pt.x + 1 + (pt.y + 1) * OutsideLocation::size].mode == TM_NORMAL)
		{
			Vec3 pos(Random(2.f) + 2.f * pt.x, 0, Random(2.f) + 2.f * pt.y);
			pos.y = terrain->GetH(pos);
			OutsideObject& o = outside_objects[Rand() % n_outside_objects];
			gameLevel->SpawnObjectEntity(locPart, o.obj, pos, Random(MAX_ANGLE), o.scale.Random());
		}
	}
}

//=================================================================================================
void CityGenerator::SpawnUnits()
{
	if(city->citizens == 0)
		return;

	LocationPart& locPart = *city;

	for(CityBuilding& b : city->buildings)
	{
		UnitData* ud = b.building->unit;
		if(!ud)
			continue;

		Vec3 pos = Vec3(float(b.unitPt.x) * 2 + 1, 0, float(b.unitPt.y) * 2 + 1);
		float rot = DirToRot(b.dir);
		Unit* u = gameLevel->CreateUnitWithAI(*city, *ud, -2, &pos, &rot);

		if(b.building->group == BuildingGroup::BG_ARENA)
			city->arenaPos = u->pos;
	}

	UnitData* dweller = UnitData::Get(city->IsVillage() ? "villager" : "citizen");

	// pijacy w karczmie
	for(int i = 0, count = Random(1, city->citizens / 3); i < count; ++i)
	{
		if(!gameLevel->SpawnUnitInsideInn(*dweller, -2))
			break;
	}

	// w�druj�cy mieszka�cy
	const int a = int(0.15f * OutsideLocation::size) + 3;
	const int b = int(0.85f * OutsideLocation::size) - 3;

	for(int i = 0; i < city->citizens; ++i)
	{
		for(int j = 0; j < 50; ++j)
		{
			Int2 pt(Random(a, b), Random(a, b));
			if(city->tiles[pt(OutsideLocation::size)].IsRoadOrPath())
			{
				gameLevel->SpawnUnitNearLocation(locPart, Vec3(2.f * pt.x + 1, 0, 2.f * pt.y + 1), *dweller, nullptr, -2, 2.f);
				break;
			}
		}
	}

	// stra�nicy
	UnitData* guard = UnitData::Get("guard_move");
	uint guard_count;
	switch(city->target)
	{
	case VILLAGE:
	case VILLAGE_EMPTY:
		guard_count = 3;
		break;
	default:
	case CITY:
		guard_count = 6;
		break;
	case CAPITAL:
		guard_count = 9;
		break;
	}
	for(uint i = 0; i < guard_count; ++i)
	{
		for(int j = 0; j < 50; ++j)
		{
			Int2 pt(Random(a, b), Random(a, b));
			if(city->tiles[pt(OutsideLocation::size)].IsRoadOrPath())
			{
				gameLevel->SpawnUnitNearLocation(locPart, Vec3(2.f * pt.x + 1, 0, 2.f * pt.y + 1), *guard, nullptr, -2, 2.f);
				break;
			}
		}
	}
}

//=================================================================================================
void CityGenerator::SpawnTemporaryUnits()
{
	if(city->citizens == 0)
		return;

	InsideBuilding* inn = city->FindInn();
	CityBuilding* training_grounds = city->FindBuilding(BuildingGroup::BG_TRAINING_GROUNDS);

	// heroes
	uint count;
	Int2 level;
	switch(city->target)
	{
	case VILLAGE:
	case VILLAGE_EMPTY:
		count = Random(1u, 2u);
		level = Int2(5, 15);
		break;
	case CITY:
		count = Random(1u, 4u);
		level = Int2(6, 15);
		break;
	case CAPITAL:
		count = 4u;
		level = Int2(8, 15);
		break;
	}

	for(uint i = 0; i < count; ++i)
	{
		UnitData& ud = *Class::GetRandomHeroData();

		if(Rand() % 2 == 0 || !training_grounds)
		{
			// inside inn
			gameLevel->SpawnUnitInsideInn(ud, level.Random(), inn, true);
		}
		else
		{
			// on training grounds
			Unit* u = gameLevel->SpawnUnitNearLocation(*city, Vec3(2.f * training_grounds->unitPt.x + 1, 0, 2.f * training_grounds->unitPt.y + 1), ud, nullptr,
				level.Random(), 8.f);
			if(u)
				u->temporary = true;
		}
	}

	// quest traveler (100% chance in city, 50% in village)
	if(!city->IsVillage() || Rand() % 2 == 0)
		gameLevel->SpawnUnitInsideInn(*UnitData::Get("traveler"), -2, inn, Level::SU_TEMPORARY);
}

//=================================================================================================
void CityGenerator::RemoveTemporaryUnits()
{
	for(LocationPart& locPart : gameLevel->ForEachPart())
	{
		DeleteElements(locPart.units, [](Unit* u)
		{
			if(!u->temporary)
				return false;
			if(u->IsHero() && u->hero->otherTeam)
				u->hero->otherTeam->Remove();
			return true;
		});
	}
}

//=================================================================================================
void CityGenerator::RepositionUnits()
{
	const int a = int(0.15f * OutsideLocation::size) + 3;
	const int b = int(0.85f * OutsideLocation::size) - 3;

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
					gameLevel->WarpUnit(u, Vec3(2.f * pt.x + 1, 0, 2.f * pt.y + 1));
					break;
				}
			}
		}
	}
}

//=================================================================================================
void CityGenerator::GeneratePickableItems()
{
	if(city->citizens == 0)
		return;

	BaseObject* table = BaseObject::Get("table"),
		*shelves = BaseObject::Get("shelves");
	vector<ItemSlot> items;

	// alcohol in inn
	InsideBuilding& inn = *city->FindInn();
	Stock* stock_table = Stock::Get("inn_on_table");
	Stock* stock_shelve = Stock::Get("inn_on_shelve");
	for(vector<Object*>::iterator it = inn.objects.begin(), end = inn.objects.end(); it != end; ++it)
	{
		Object& obj = **it;
		if(obj.base == table)
			gameLevel->PickableItemsFromStock(inn, obj, *stock_table);
		else if(obj.base == shelves)
			gameLevel->PickableItemsFromStock(inn, obj, *stock_shelve);
	}

	// food in food shop
	CityBuilding* food = city->FindBuilding(BuildingGroup::BG_FOOD_SELLER);
	if(food)
	{
		Object* found_obj = city->FindNearestObject(shelves, food->walkPt);
		if(found_obj)
		{
			Stock* stock = Stock::Get("foodseller_shelve");
			gameLevel->PickableItemsFromStock(*city, *found_obj, *stock);
		}
	}

	// potions in alchemy shop
	CityBuilding* alch = city->FindBuilding(BuildingGroup::BG_ALCHEMIST);
	if(alch)
	{
		Object* found_obj = city->FindNearestObject(shelves, alch->walkPt);
		if(found_obj)
		{
			Stock* stock = Stock::Get("alchemist_shelve");
			gameLevel->PickableItemsFromStock(*city, *found_obj, *stock);
		}
	}
}

//=================================================================================================
void CityGenerator::CreateMinimap()
{
	DynamicTexture& tex = *game->tMinimap;
	tex.Lock();

	for(int y = 0; y < OutsideLocation::size; ++y)
	{
		uint* pix = tex[y];
		for(int x = 0; x < OutsideLocation::size; ++x)
		{
			const TerrainTile& t = city->tiles[x + (OutsideLocation::size - 1 - y) * OutsideLocation::size];
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

	tex.Unlock();
	gameLevel->minimapSize = OutsideLocation::size;
}

//=================================================================================================
void CityGenerator::OnLoad()
{
	SetOutsideParams();
	game->SetTerrainTextures();
	ApplyTiles();
	SetBuildingsParams();

	gameLevel->RecreateObjects(Net::IsClient());
	gameLevel->SpawnTerrainCollider();
	RespawnBuildingPhysics();
	SpawnCityPhysics();
	SpawnOutsideBariers();
	game->InitQuadTree();
	game->CalculateQuadtree();

	CreateMinimap();
}

//=================================================================================================
void CityGenerator::RespawnBuildingPhysics()
{
	for(vector<CityBuilding>::iterator it = city->buildings.begin(), end = city->buildings.end(); it != end; ++it)
	{
		Building* b = it->building;
		gameLevel->ProcessBuildingObjects(*city, city, nullptr, b->mesh, nullptr, DirToRot(it->dir), it->dir,
			Vec3(float(it->pt.x + b->shift[it->dir].x) * 2, 1.f, float(it->pt.y + b->shift[it->dir].y) * 2), nullptr, &*it, true);
	}

	for(vector<InsideBuilding*>::iterator it = city->insideBuildings.begin(), end = city->insideBuildings.end(); it != end; ++it)
	{
		gameLevel->ProcessBuildingObjects(**it, city, *it, (*it)->building->insideMesh, nullptr, 0.f, GDIR_DOWN,
			Vec3((*it)->offset.x, 0.f, (*it)->offset.y), nullptr, nullptr, true);
	}
}

//=================================================================================================
void CityGenerator::SetBuildingsParams()
{
	for(InsideBuilding* insideBuilding : city->insideBuildings)
	{
		Scene* scene = insideBuilding->lvlPart->scene;
		scene->clearColor = Color::White;
		scene->fogRange = Vec2(40, 80);
		scene->fogColor = Color(0.9f, 0.85f, 0.8f);
		scene->ambientColor = Color(0.5f, 0.5f, 0.5f);
		if(insideBuilding->top > 0.f)
			scene->useLightDir = false;
		else
		{
			scene->lightColor = Color::White;
			scene->lightDir = Vec3(sin(gameLevel->lightAngle), 2.f, cos(gameLevel->lightAngle)).Normalize();
			scene->useLightDir = true;
		}
		insideBuilding->lvlPart->drawRange = 80.f;
	}
}

//=================================================================================================
void CityGenerator::SpawnUnits(UnitGroup* group, int level)
{
	const int a = int(0.15f * OutsideLocation::size) + 3;
	const int b = int(0.85f * OutsideLocation::size) - 3;
	LocalVector3<Vec3> usedPositions;
	usedPositions.push_back(teamPos);
	Pooled<TmpUnitGroup> tmpGroup;
	tmpGroup->Fill(group, level);

	for(int i = 0; i < 8; ++i)
	{
		for(int j = 0; j < 50; ++j)
		{
			const Int2 pt(Random(a, b), Random(a, b));
			if(city->tiles[pt(OutsideLocation::size)].IsRoadOrPath())
			{
				bool ok = true;
				const Vec3 pos = PtToPos(pt);

				for(const Vec3& usedPos : usedPositions)
				{
					if(Vec3::DistanceSquared(pos, usedPos) < Pow2(24.f))
					{
						ok = false;
						break;
					}
				}

				if(ok)
				{
					usedPositions.push_back(pos);
					for(TmpUnitGroup::Spawn& spawn : tmpGroup->Roll(level, 2))
					{
						if(!gameLevel->SpawnUnitNearLocation(*city, pos, *spawn.first, nullptr, spawn.second, 6.f))
							break;
					}
					break;
				}
			}
		}
	}
}
