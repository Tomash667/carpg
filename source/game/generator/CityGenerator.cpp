#include "Pch.h"
#include "Base.h"
#include "CityGenerator.h"

const float SPAWN_RATIO = 0.2f;
const float SPAWN_RANGE = 4.f;
const float EXIT_WIDTH = 1.3f;
const float EXIT_START = 11.1f;
const float EXIT_END = 12.6f;

//=================================================================================================
void CityGenerator::Init(TerrainTile* _tiles, float* _height, int _w, int _h)
{
	assert(_tiles && _height && _w>0 && _h>0);
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
	rs1 = road_size/2;
	rs2 = road_size/2;
	road_part = _road_part;
}

//=================================================================================================
void CityGenerator::SetTerrainNoise(int octaves, float frequency, float _hmin, float _hmax)
{
	perlin.Change(max(w,h), octaves, frequency, 1.f);
	hmin = _hmin;
	hmax = _hmax;
}

//=================================================================================================
void CityGenerator::GenerateMainRoad(RoadType type, GAME_DIR dir, int rocky_roads, bool plaza, int swap, vector<EntryPoint>& entry_points, int& gates, bool fill_roads)
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
			AddRoad(INT2(0,h/2), INT2(w/2,h/2), ROAD_HORIZONTAL | ROAD_START_CHECKED | ROAD_END_CHECKED);
			AddRoad(INT2(w/2,h/2), INT2(w,h/2), ROAD_HORIZONTAL | ROAD_START_CHECKED | ROAD_END_CHECKED);
			AddRoad(INT2(w/2,0), INT2(w/2,h/2), ROAD_START_CHECKED | ROAD_END_CHECKED);
			AddRoad(INT2(w/2,h/2), INT2(w/2,h), ROAD_START_CHECKED | ROAD_END_CHECKED);
		}
		break;

	case RoadType_Line:
		if(dir == GDIR_LEFT || dir == GDIR_RIGHT)
		{
			gates = GATE_EAST | GATE_WEST;
			if(fill_roads)
			{
				AddRoad(INT2(0,h/2), INT2(w/2,h/2), ROAD_HORIZONTAL | ROAD_START_CHECKED | ROAD_END_CHECKED);
				AddRoad(INT2(w/2,h/2), INT2(w,h/2), ROAD_HORIZONTAL | ROAD_START_CHECKED | ROAD_END_CHECKED);
			}
		}
		else
		{
			gates = GATE_NORTH | GATE_SOUTH;
			if(fill_roads)
			{
				AddRoad(INT2(w/2,0), INT2(w/2,h/2), ROAD_START_CHECKED | ROAD_END_CHECKED);
				AddRoad(INT2(w/2,h/2), INT2(w/2,h), ROAD_START_CHECKED | ROAD_END_CHECKED);
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

			INT2 pts[3];
			pts[1] = INT2(w/2,h/2);
			TERRAIN_TILE t = (rocky_roads > 0 ? TT_ROAD : TT_SAND);

			switch(dir)
			{
			case GDIR_LEFT:
				CreateRoad(Rect(0,H1,road_part,H2), t);
				CreateEntry(entry_points, ED_Left);
				pts[0] = INT2(road_part,h/2);
				if(!swap)
				{
					CreateRoad(Rect(W1,0,W2,road_part), t);
					CreateEntry(entry_points, ED_Bottom);
					pts[2] = INT2(w/2,road_part);
				}
				else
				{
					CreateRoad(Rect(W1,h-road_part,W2,h-1), t);
					CreateEntry(entry_points, ED_Top);
					pts[2] = INT2(w/2,h-road_part);
				}
				break;
			case GDIR_RIGHT:
				CreateRoad(Rect(w-road_part,H1,w-1,H2), t);
				CreateEntry(entry_points, ED_Right);
				pts[0] = INT2(w-road_part,h/2);
				if(!swap)
				{
					CreateRoad(Rect(W1,0,W2,road_part), t);
					CreateEntry(entry_points, ED_Bottom);
					pts[2] = INT2(w/2,road_part);
				}
				else
				{
					CreateRoad(Rect(W1,h-road_part,W2,h-1), t);
					CreateEntry(entry_points, ED_Top);
					pts[2] = INT2(w/2,h-road_part);
				}
				break;
			case GDIR_DOWN:
				CreateRoad(Rect(W1,0,W2,road_part), t);
				CreateEntry(entry_points, ED_Bottom);
				pts[0] = INT2(w/2,road_part);
				if(!swap)
				{
					CreateRoad(Rect(0,H1,road_part,H2), t);
					CreateEntry(entry_points, ED_Left);
					pts[2] = INT2(road_part,h/2);
				}
				else
				{
					CreateRoad(Rect(w-road_part,H1,w-1,H2), t);
					CreateEntry(entry_points, ED_Right);
					pts[2] = INT2(w-road_part,h/2);
				}
				break;
			case GDIR_UP:
				CreateRoad(Rect(W1,h-road_part,W2,h-1), t);
				CreateEntry(entry_points, ED_Top);
				pts[0] = INT2(w/2,h-road_part);
				if(!swap)
				{
					CreateRoad(Rect(0,H1,road_part,H2), t);
					CreateEntry(entry_points, ED_Left);
					pts[2] = INT2(road_part,h/2);
				}
				else
				{
					CreateRoad(Rect(w-road_part,H1,w-1,H2), t);
					CreateEntry(entry_points, ED_Right);
					pts[2] = INT2(w-road_part,h/2);
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
				CreateRoad(Rect(0,H1,road_part,H2), t);
				CreateEntry(entry_points, ED_Left);
				CreateRoad(Rect(w-road_part,H1,w-1,H2), t);
				CreateEntry(entry_points, ED_Right);
				INT2 pts[3];
				pts[0] = INT2(road_part,h/2);
				pts[1] = INT2(w/2,road_part);
				pts[2] = INT2(w-road_part,h/2);
				CreateCurveRoad(pts, 3, t);
				pts[1] = INT2(w/2,h-road_part);
				CreateCurveRoad(pts, 3, t);
			}
			else
			{
				gates = GATE_NORTH | GATE_SOUTH;
				CreateRoad(Rect(W1,0,W2,road_part), t);
				CreateEntry(entry_points, ED_Bottom);
				CreateRoad(Rect(W1,h-road_part,W2,h-1), t);
				CreateEntry(entry_points, ED_Top);
				INT2 pts[3];
				pts[0] = INT2(w/2,road_part);
				pts[1] = INT2(road_part,h/2);
				pts[2] = INT2(w/2,h-road_part);
				CreateCurveRoad(pts, 3, t);
				pts[1] = INT2(w-road_part,h/2);
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
			TERRAIN_TILE t[3] = {TT_ROAD, rocky_roads > 1 ? TT_ROAD : TT_SAND, TT_SAND};
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
				AddRoad(INT2(0,h/2), INT2(w/2,h/2), ROAD_HORIZONTAL | ROAD_START_CHECKED | ROAD_END_CHECKED);
			if(dir != GDIR_RIGHT)
				AddRoad(INT2(w/2,h/2), INT2(w,h/2), ROAD_HORIZONTAL | ROAD_START_CHECKED | ROAD_END_CHECKED);
			if(dir != GDIR_DOWN)
				AddRoad(INT2(w/2,0), INT2(w/2,h/2), ROAD_START_CHECKED | ROAD_END_CHECKED);
			if(dir != GDIR_UP)
				AddRoad(INT2(w/2,h/2), INT2(w/2,h), ROAD_START_CHECKED | ROAD_END_CHECKED);
		}
		break;
		
	case RoadType_Sin:
		{
			gates = 0;

			TERRAIN_TILE t = (rocky_roads != 0 ? TT_ROAD : TT_SAND);
			INT2 pts[4];
			switch(dir)
			{
			case GDIR_LEFT:
				CreateRoad(Rect(0,h/2-road_part-rs1,road_part,h/2-road_part+rs2), t);
				CreateEntry(entry_points, ED_LeftBottom);
				CreateRoad(Rect(w-road_part,h/2+road_part-rs1,w-1,h/2+road_part+rs2), t);
				CreateEntry(entry_points, ED_RightTop);
				pts[0] = INT2(road_part,h/2-road_part);
				pts[1] = INT2(w/2,h/2-road_part);
				pts[2] = INT2(w/2,h/2+road_part);
				pts[3] = INT2(w-road_part,h/2+road_part);
				break;
			case GDIR_RIGHT:
				CreateRoad(Rect(w-road_part,h/2-road_part-rs1,w-1,h/2-road_part+rs2), t);
				CreateEntry(entry_points, ED_RightBottom);
				CreateRoad(Rect(0,h/2+road_part-rs1,road_part,h/2+road_part+rs2), t);
				CreateEntry(entry_points, ED_LeftTop);
				pts[0] = INT2(w-road_part,h/2-road_part);
				pts[2] = INT2(w/2,h/2+road_part);
				pts[1] = INT2(w/2,h/2-road_part);
				pts[3] = INT2(road_part,h/2+road_part);
				break;
			case GDIR_DOWN:
				CreateRoad(Rect(w/2-road_part-rs1,0,w/2-road_part+rs2,road_part), t);
				CreateEntry(entry_points, ED_BottomLeft);
				CreateRoad(Rect(w/2+road_part-rs1,h-road_part,w/2+road_part+rs2,h-1), t);
				CreateEntry(entry_points, ED_TopRight);
				pts[0] = INT2(w/2-road_part,road_part);
				pts[1] = INT2(w/2-road_part,h/2);
				pts[2] = INT2(w/2+road_part,h/2);
				pts[3] = INT2(w/2+road_part,h-road_part);
				break;
			case GDIR_UP:
				CreateRoad(Rect(w/2-road_part-rs1,h-road_part,w/2-road_part+rs2,h-1), t);
				CreateEntry(entry_points, ED_TopLeft);
				CreateRoad(Rect(w/2+road_part-rs1,0,w/2+road_part+rs2,road_part), t);
				CreateEntry(entry_points, ED_BottomRight);
				pts[0] = INT2(w/2-road_part,h-road_part);
				pts[1] = INT2(w/2-road_part,h/2);
				pts[2] = INT2(w/2+road_part,h/2);
				pts[3] = INT2(w/2+road_part,road_part);
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
		INT2 center(w/2,h/2);
		for(int y=-5; y<=5; ++y)
		{
			for(int x=-5; x<=5; ++x)
			{
				INT2 pt = INT2(center.x-x,center.y-y);
				if(distance(pt_to_pos(pt), pt_to_pos(center)) <= 10.f)
					tiles[pt(w)].Set(rocky_roads > 0 ? TT_ROAD : TT_SAND, TT_GRASS, 0, TM_ROAD);
			}
		}
	}

	if(!roads.empty())
	{
		for(int i = 0; i < (int)roads.size(); ++i)
		{
			const Road2& r = roads[i];
			int minx = max(0, r.start.x-1),
				miny = max(0, r.start.y-1),
				maxx = min(w-1, r.end.x+1),
				maxy = min(h-1, r.end.y+1);
			for(int y = miny; y <= maxy; ++y)
			{
				for(int x = minx; x <= maxx; ++x)
					road_ids[x + y*w] = i;
			}
		}
	}
}

//=================================================================================================
void CityGenerator::CreateRoad(const Rect& r, TERRAIN_TILE t)
{
	assert(r.minx >=0 && r.miny >= 0 && r.maxx < w && r.maxy < h);
	for(int y=r.miny; y<=r.maxy; ++y)
	{
		for(int x=r.minx; x<=r.maxx; ++x)
		{
			TerrainTile& tt = tiles[x+y*w];
			tt.t = t;
			tt.t2 = t;
			tt.alpha = 0;
			tt.mode = TM_ROAD;
		}
	}
}

//=================================================================================================
// nie obs³uguje krawêdzi!
void CityGenerator::CreateCurveRoad(INT2 points[], uint count, TERRAIN_TILE t)
{
	assert(count == 3 || count == 4);

	if(count == 3)
		PlotQuadBezier(points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y, 1.f, (float)road_size, pixels);
	else
		PlotCubicBezier(points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y, points[3].x, points[3].y, (float)road_size, pixels);

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
			tile.alpha = byte((float(tile.alpha)/255*float(it->alpha)/255)*255);
	}
	pixels.clear();
}

//=================================================================================================
void CityGenerator::GenerateBuildings(vector<ToBuild>& tobuild)
{
	// budynki
	for(vector<ToBuild>::iterator build_it = tobuild.begin(), build_end = tobuild.end(); build_it != build_end; ++build_it)
	{
		INT2 ext = build_it->type->size - INT2(1,1);

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
		for(int y=ymin; y<ymax; ++y)
		{
			for(int x=ymin; x<ymax; ++x)
			{

#define CHECK_TILE(_x,_y) ok = true; \
	for(int _yy=-1; _yy<=1 && ok; ++_yy) { \
	for(int _xx=-1; _xx<=1; ++_xx) { \
	if(tiles[(_x)+_xx+((_y)+_yy)*w].t != TT_GRASS) { ok = false; break; } } }

				CHECK_TILE(x,y);
				if(!ok)
					continue;

				if(ext.x == ext.y)
				{
					int a = ext.x/2;
					int b = ext.x-a;

					for(int yy=-a; yy<=b; ++yy)
					{
						for(int xx=-a; xx<=b; ++xx)
						{
							CHECK_TILE(x+xx,y+yy);
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
					int a1 = ext.x/2;
					int b1 = ext.x-a1;
					int a2 = ext.y/2;
					int b2 = ext.y-a2;

					side = -1;

					// po x
					for(int yy=-a1; yy<=b1; ++yy)
					{
						for(int xx=-a2; xx<=b2; ++xx)
						{
							CHECK_TILE(x+xx,y+yy);
							if(!ok)
								goto superbreak2;
						}
					}
superbreak2:

					if(ok)
						side = 0;

					// po z
					for(int yy=-a2; yy<=b2; ++yy)
					{
						for(int xx=-a1; xx<=b1; ++xx)
						{
							CHECK_TILE(x+xx,y+yy);
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
					bpt.pt = INT2(x,y);
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
			ERROR(Format("Failed to generate city map! No place for building %d!", build_it->type));
			return;
		}

		// ustal pozycjê i obrót budynku
		const INT2 centrum(w/2,w/2);
		int range, best_range = INT_MAX, index = 0;
		for(vector<BuildPt>::iterator it = points.begin(), end = points.end(); it != end; ++it, ++index)
		{
			int best_length = 999, dir = -1;

			// calculate distance to closest road
			if(it->side == 1 || it->side == 2)
			{
				// down
				int length = 1;
				INT2 pt = it->pt;
				--pt.y;

				while(1)
				{
					TerrainTile& t = tiles[pt.x+pt.y*w];
					if(t.mode == TM_ROAD)
					{
						if(t.t == TT_SAND)
							length = length*2+5;
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

				if(tiles[pt.x+pt.y*w].mode == TM_ROAD && length < best_length)
				{
					best_length = length;
					dir = 0;
				}

				// up
				length = 1;
				pt = it->pt;
				++pt.y;

				while(1)
				{
					TerrainTile& t = tiles[pt.x+pt.y*w];
					if(t.mode == TM_ROAD)
					{
						if(t.t == TT_SAND)
							length = length*2+5;
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

				if(tiles[pt.x+pt.y*w].mode == TM_ROAD && length < best_length)
				{
					best_length = length;
					dir = 2;
				}
			}
			if(it->side == 0 || it->side == 2)
			{
				// left
				int length = 1;
				INT2 pt = it->pt;
				--pt.x;

				while(1)
				{
					TerrainTile& t = tiles[pt.x+pt.y*w];
					if(t.mode == TM_ROAD)
					{
						if(t.t == TT_SAND)
							length = length*2+5;
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

				if(tiles[pt.x+pt.y*w].mode == TM_ROAD && length < best_length)
				{
					best_length = length;
					dir = 3;
				}

				// right
				length = 1;
				pt = it->pt;
				++pt.x;

				while(1)
				{
					TerrainTile& t = tiles[pt.x+pt.y*w];
					if(t.mode == TM_ROAD)
					{
						if(t.t == TT_SAND)
							length = length*2+5;
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

				if(tiles[pt.x+pt.y*w].mode == TM_ROAD && length < best_length)
				{
					best_length = length;
					dir = 1;
				}
			}
			
			if(dir == -1)
			{
				if(it->side == 2)
					dir = rand2()%4;
				else if(it->side == 1)
				{
					if(rand2()%2 == 0)
						dir = 0;
					else
						dir = 2;
				}
				else
				{
					if(rand2()%2 == 0)
						dir = 1;
					else
						dir = 3;
				}
			}

			if(IS_SET(build_it->type->flags, Building::FAVOR_CENTER))
				range = distance(centrum, it->pt);
			else
				range = 0;
			if(IS_SET(build_it->type->flags, Building::FAVOR_ROAD))
				range += max(0, best_length-1);
			else
				range += max(0, best_length-5);

			if(range <= best_range)
			{
				if(range < best_range)
				{
					valid_pts.clear();
					best_range = range;
				}
				BuildPt p;
				p.pt = it->pt;
				p.side = dir;
				valid_pts.push_back(p);
			}
		}

		BuildPt pt = random_item(valid_pts);
		int best_dir = pt.side;
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

		build_it->pt = pt.pt;
		build_it->rot = best_dir;

		INT2 ext2 = build_it->type->size;
		if(best_dir == 1 || best_dir == 3)
			std::swap(ext2.x, ext2.y);

		const int x1 = (ext2.x-1)/2;
		const int x2 = ext2.x-x1-1;
		const int y1 = (ext2.y-1)/2;
		const int y2 = ext2.y-y1-1;

		INT2 road_start(-1,-1);
		int count = 0;
		float sum = 0;

		for(int yy=-y1, yr=0; yy<=y2; ++yy, ++yr)
		{
			for(int xx=-x1, xr=0; xx<=x2; ++xx, ++xr)
			{
				Building::TileScheme co;
				switch(best_dir)
				{
				case 0:
					co = build_it->type->scheme[xr+(ext2.y-yr-1)*ext2.x];
					break;
				case 1:
					co = build_it->type->scheme[yr+xr*ext2.y];
					break;
				case 2:
					co = build_it->type->scheme[ext2.x-xr-1+yr*ext2.x];
					break;
				case 3:
					co = build_it->type->scheme[ext2.y-yr-1+(ext2.x-xr-1)*ext2.y];
					break;
				default:
					assert(0);
					break;
				}

				INT2 pt2(pt.pt.x+xx, pt.pt.y+yy);

				TerrainTile& t = tiles[pt2.x+(pt2.y)*w];
				assert(t.t == TT_GRASS);

				switch(co)
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
					assert(road_start == INT2(-1,-1));
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

		for(int yy=-y1, yr=0; yy<=y2+1; ++yy, ++yr)
		{
			for(int xx=-x1, xr=0; xx<=x2+1; ++xx, ++xr)
			{
				INT2 pt2(pt.pt.x+xx, pt.pt.y+yy);
				++count;
				sum += height[pt2.x+pt2.y*(w+1)];
				tmp_pts.push_back(pt2);
			}
		}

		// set height
		sum /= count;
		for(INT2& pt : tmp_pts)
			height[pt.x+pt.y*(w+1)] = sum;
		tmp_pts.clear();

		assert(road_start != INT2(-1,-1));

		if(road_start != INT2(-1,-1))
			GeneratePath(road_start);
	}
}

//=================================================================================================
void CityGenerator::GeneratePath(const INT2& pt)
{
	assert(pt.x >= 0 && pt.y >= 0 && pt.x < w && pt.y < h);

	int size = w*h;
	if(size > (int)grid.size())
		grid.resize(size);
	memset(&grid[0], 0, sizeof(APoint2)*size);
	
	int start_idx = pt.x+pt.y*w;
	to_check.push_back(start_idx);

	grid[start_idx].stan = 1;
	grid[start_idx].dir = -1;

	struct Mod
	{
		INT2 change, back;
		int cost, cost2;

		Mod(const INT2& change, const INT2& back, int cost, int cost2) : change(change), back(back), cost(cost), cost2(cost2)
		{

		}
	};
	static const Mod mod[8] = {
		Mod(INT2(-1,0), INT2(1,0), 10, 15),
		Mod(INT2(1,0), INT2(-1,0), 10, 15),
		Mod(INT2(0,-1), INT2(0,1), 10, 15),
		Mod(INT2(0,1), INT2(0,-1), 10, 15),
		Mod(INT2(-1,-1), INT2(1,1), 20, 30),
		Mod(INT2(-1,1), INT2(1,-1), 20, 30),
		Mod(INT2(1,-1), INT2(-1,1), 20, 30),
		Mod(INT2(1,1), INT2(-1,-1), 20, 30)
	};

	int end_tile_idx = -1;
	const int x_min = int(float(w)*0.2f);
	const int x_max = int(float(w)*0.8f);
	const int y_min = int(float(h)*0.2f);
	const int y_max = int(float(h)*0.8f);

	while(!to_check.empty())
	{
		int pt_idx = to_check.back();
		INT2 pt(pt_idx%w,pt_idx/w);
		to_check.pop_back();
		if(pt.x <= x_min || pt.x >= x_max || pt.y <= y_min || pt.y >= y_max)
			continue;

		APoint2& this_point = grid[pt_idx];

		for(int i=0; i<8; ++i)
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

	INT2 pt2(end_tile_idx % w, end_tile_idx / w);
	bool go = true;
	while(go)
	{
		TerrainTile& t = tiles[pt2.x+pt2.y*w];
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
		const APoint2& apt = grid[pt2.x+pt2.y*w];
		if(apt.dir > 3)
		{
			const Mod& m = mod[apt.dir];
			{
				TerrainTile& t = tiles[pt2.x+m.back.x+pt2.y*w];
				if(t.t == TT_GRASS)
					t.Set(TT_SAND, TT_GRASS, 96, TM_PATH);
				else if(t.mode == TM_ROAD)
				{
					if(t.t == TT_ROAD)
						t.t2 = TT_SAND;
					else if(t.alpha > 96)
						t.alpha = (t.alpha+96)/2;
				}
			}
			{
				TerrainTile& t = tiles[pt2.x+(pt2.y+m.back.y)*w];
				if(t.t == TT_GRASS)
					t.Set(TT_SAND, TT_GRASS, 96, TM_PATH);
				else if(t.mode == TM_ROAD)
				{
					if(t.t == TT_ROAD)
						t.t2 = TT_SAND;
					else if(t.alpha > 96)
						t.alpha = (t.alpha+96)/2;
				}
			}
		}
		pt2 = apt.prev;
	}
}

//=================================================================================================
void CityGenerator::RandomizeHeight()
{
	float hdif = hmax - hmin;
	const float hm = sqrt(2.f)/2;

	for(int y=0; y<=h; ++y)
	{
		for(int x=0; x<=w; ++x)
		{
			float a = perlin.Get(1.f/(w+1)*x, 1.f/(h+1)*y);
			height[x+y*(w+1)] = (a+hm)/(hm*2)*hdif+hmin;
		}
	}

	for(int y=0; y<h; ++y)
	{
		for(int x=0; x<w; ++x)
		{
			if(x < 15 || x > w-15 || y < 15 || y > h-15)
				height[x+y*(w+1)] += random(5.f,10.f);
		}
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
const INT2 blocked[] = {
	INT2(0,0),
	INT2(-1,0),
	INT2(0,1),
	INT2(1,0),
	INT2(0,-1),
	INT2(-1,-1),
	INT2(-1,1),
	INT2(1,1),
	INT2(1,-1),
	INT2(-1,2),
	INT2(0,2),
	INT2(1,2),
	INT2(2,1),
	INT2(2,0),
	INT2(2,-1),
	INT2(1,-2),
	INT2(0,-2),
	INT2(-1,-2),
	INT2(-2,-1),
	INT2(-2,0),
	INT2(-2,1)
};
struct Nei
{
	INT2 pt;
	int id[4];
};
const Nei neis[] = {
	INT2(0,0), 1, 5, 4, 0,
	INT2(0,1), 1, 6, 2, 0,
	INT2(1,1), 0, 2, 7, 3,
	INT2(1,0), 0, 3, 4, 8,
	INT2(-1,0), 18, 19, 1, 5,
	INT2(-1,1), 19, 20, 6, 1,
	INT2(0,2), 6, 9, 10, 2,
	INT2(1,2), 2, 10, 11, 7,
	INT2(2,1), 3, 7, 12, 13,
	INT2(2,0), 8, 3, 13, 14,
	INT2(1,-1), 16, 4, 8, 15,
	INT2(0,-1), 17, 5, 4, 16
};

//=================================================================================================
void CityGenerator::FlattenRoad()
{
	const int w1 = w+1;
	bool block[21];
	block[0] = true;

	for(int y=2; y<h-2; ++y)
	{
		for(int x=2; x<w-2; ++x)
		{
			if(OR2_EQ(tiles[x+y*w].mode, TM_ROAD, TM_PATH))
			{
				for(int i=1; i<21; ++i)
					block[i] = !tiles[x+blocked[i].x+(y+blocked[i].y)*w].IsBuilding();

				float sum = 0.f;
				for(int i=0; i<12; ++i)
				{
					const Nei& nei = neis[i];
					INT2 pt(x + nei.pt.x, y + nei.pt.y);
					sum += height[pt(w1)];
					if(block[nei.id[0]] && block[nei.id[1]] && block[nei.id[2]] && block[nei.id[3]])
						tmp_pts.push_back(pt);
				}

				sum /= 12;
				for(INT2& pt : tmp_pts)
					height[pt(w1)] = sum;
				tmp_pts.clear();
			}
		}
	}
}

//=================================================================================================
void CityGenerator::SmoothTerrain()
{
	for(int y=1; y<h; ++y)
	{
		for(int x=1; x<w; ++x)
		{
			if(tiles[x+y*w].mode < TM_BUILDING_SAND && tiles[x-1+y*w].mode < TM_BUILDING_SAND && tiles[x+(y-1)*w].mode < TM_BUILDING_SAND && tiles[x-1+(y-1)*w].mode < TM_BUILDING_SAND)
			{
				float sum = (height[x+y*(w+1)] + height[x-1+y*(w+1)] + height[x+1+y*(w+1)] + height[x+(y-1)*(h+1)] + height[x+(y+1)*(h+1)])/5;
				height[x+y*(w+1)] = sum;
			}
		}
	}
}

//=================================================================================================
void CityGenerator::CreateRoadLineLeftRight(TERRAIN_TILE t, vector<EntryPoint>& entry_points)
{
	CreateRoad(Rect(0, h/2-rs1, w-1, h/2+rs2), t);
	CreateEntry(entry_points, ED_Left);
	CreateEntry(entry_points, ED_Right);
}

//=================================================================================================
void CityGenerator::CreateRoadLineBottomTop(TERRAIN_TILE t, vector<EntryPoint>& entry_points)
{
	CreateRoad(Rect(w/2-rs1, 0, w/2+rs2, h-1), t);
	CreateEntry(entry_points, ED_Bottom);
	CreateEntry(entry_points, ED_Top);
}

//=================================================================================================
void CityGenerator::CreateRoadPartLeft(TERRAIN_TILE t, vector<EntryPoint>& entry_points)
{
	CreateRoad(Rect(0, h/2-rs1, w/2-rs1-1, h/2+rs2), t);
	CreateEntry(entry_points, ED_Left);
}

//=================================================================================================
void CityGenerator::CreateRoadPartRight(TERRAIN_TILE t, vector<EntryPoint>& entry_points)
{
	CreateRoad(Rect(w/2+rs2+1,h/2-rs1,w-1,h/2+rs2), t);
	CreateEntry(entry_points, ED_Right);
}

//=================================================================================================
void CityGenerator::CreateRoadPartBottom(TERRAIN_TILE t, vector<EntryPoint>& entry_points)
{
	CreateRoad(Rect(w/2-rs1,0,w/2+rs1,h/2-rs1-1), t);
	CreateEntry(entry_points, ED_Bottom);
}

//=================================================================================================
void CityGenerator::CreateRoadPartTop(TERRAIN_TILE t, vector<EntryPoint>& entry_points)
{
	CreateRoad(Rect(w/2-rs1,h/2+rs2+1,w/2+rs2,h-1), t);
	CreateEntry(entry_points, ED_Top);
}

//=================================================================================================
void CityGenerator::CreateRoadCenter(TERRAIN_TILE t)
{
	CreateRoad(Rect(w/2-rs1,h/2-rs1,w/2+rs2,h/2+rs2), t);
}

//=================================================================================================
void CityGenerator::CreateEntry(vector<EntryPoint>& entry_points, EntryDir dir)
{
	EntryPoint& ep = Add1(entry_points);

	switch(dir)
	{
	case ED_Left:
		{
			VEC2 p(SPAWN_RATIO*w*2,float(h)+1);
			ep.spawn_rot = PI*3/2;
			ep.spawn_area = BOX2D(p.x-SPAWN_RANGE,p.y-SPAWN_RANGE,p.x+SPAWN_RANGE,p.y+SPAWN_RANGE);
			ep.exit_area = BOX2D(p.x-EXIT_END,p.y-EXIT_WIDTH,p.x-EXIT_START,p.y+EXIT_WIDTH);
		}
		break;
	case ED_Right:
		{
			VEC2 p((1.f-SPAWN_RATIO)*w*2,float(h)+1);
			ep.spawn_rot = PI/2;			
			ep.spawn_area = BOX2D(p.x-SPAWN_RANGE,p.y-SPAWN_RANGE,p.x+SPAWN_RANGE,p.y+SPAWN_RANGE);
			ep.exit_area = BOX2D(p.x+EXIT_START,p.y-EXIT_WIDTH,p.x+EXIT_END,p.y+EXIT_WIDTH);
		}
		break;
	case ED_Bottom:
		{
			VEC2 p(float(w)+1,SPAWN_RATIO*h*2);
			ep.spawn_rot = PI;
			ep.spawn_area = BOX2D(p.x-SPAWN_RANGE,p.y-SPAWN_RANGE,p.x+SPAWN_RANGE,p.y+SPAWN_RANGE);
			ep.exit_area = BOX2D(p.x-EXIT_WIDTH,p.y-EXIT_END,p.x+EXIT_WIDTH,p.y-EXIT_START);
		}
		break;
	case ED_Top:
		{
			VEC2 p(float(w)+1,(1.f-SPAWN_RATIO)*h*2);
			ep.spawn_rot = 0;
			ep.spawn_area = BOX2D(p.x-SPAWN_RANGE,p.y-SPAWN_RANGE,p.x+SPAWN_RANGE,p.y+SPAWN_RANGE);
			ep.exit_area = BOX2D(p.x-EXIT_WIDTH,p.y+EXIT_START,p.x+EXIT_WIDTH,p.y+EXIT_END);
		}
		break;
	case ED_LeftBottom:
		{
			VEC2 p(SPAWN_RATIO*w*2,float(h/2-road_part)*2+1);
			ep.spawn_rot = PI*3/2;
			ep.spawn_area = BOX2D(p.x-SPAWN_RANGE,p.y-SPAWN_RANGE,p.x+SPAWN_RANGE,p.y+SPAWN_RANGE);
			ep.exit_area = BOX2D(p.x-EXIT_END,p.y-EXIT_WIDTH,p.x+EXIT_START,p.y+EXIT_WIDTH);
		}
		break;
	case ED_LeftTop:
		{
			VEC2 p(SPAWN_RATIO*w*2,float(h/2+road_part)*2+1);
			ep.spawn_rot = PI*3/2;
			ep.spawn_area = BOX2D(p.x-SPAWN_RANGE,p.y-SPAWN_RANGE,p.x+SPAWN_RANGE,p.y+SPAWN_RANGE);
			ep.exit_area = BOX2D(p.x-EXIT_END,p.y-EXIT_WIDTH,p.x+EXIT_START,p.y+EXIT_WIDTH);
		}
		break;
	case ED_RightBottom:
		{
			VEC2 p((1.f-SPAWN_RATIO)*w*2,float(h/2-road_part)*2+1);
			ep.spawn_rot = PI/2;			
			ep.spawn_area = BOX2D(p.x-SPAWN_RANGE,p.y-SPAWN_RANGE,p.x+SPAWN_RANGE,p.y+SPAWN_RANGE);
			ep.exit_area = BOX2D(p.x+EXIT_START,p.y-EXIT_WIDTH,p.x+EXIT_END,p.y+EXIT_WIDTH);
		}
		break;
	case ED_RightTop:
		{
			VEC2 p((1.f-SPAWN_RATIO)*w*2,float(h/2+road_part)*2+1);
			ep.spawn_rot = PI/2;			
			ep.spawn_area = BOX2D(p.x-SPAWN_RANGE,p.y-SPAWN_RANGE,p.x+SPAWN_RANGE,p.y+SPAWN_RANGE);
			ep.exit_area = BOX2D(p.x+EXIT_START,p.y-EXIT_WIDTH,p.x+EXIT_END,p.y+EXIT_WIDTH);
		}
		break;
	case ED_BottomLeft:
		{
			VEC2 p(float(w/2-road_part)*2+1,SPAWN_RATIO*h*2);
			ep.spawn_rot = PI;
			ep.spawn_area = BOX2D(p.x-SPAWN_RANGE,p.y-SPAWN_RANGE,p.x+SPAWN_RANGE,p.y+SPAWN_RANGE);
			ep.exit_area = BOX2D(p.x-EXIT_WIDTH,p.y-EXIT_END,p.x+EXIT_WIDTH,p.y+EXIT_START);
		}
		break;
	case ED_BottomRight:
		{
			VEC2 p(float(w/2+road_part)*2+1,SPAWN_RATIO*h*2);
			ep.spawn_rot = PI;
			ep.spawn_area = BOX2D(p.x-SPAWN_RANGE,p.y-SPAWN_RANGE,p.x+SPAWN_RANGE,p.y+SPAWN_RANGE);
			ep.exit_area = BOX2D(p.x-EXIT_WIDTH,p.y-EXIT_END,p.x+EXIT_WIDTH,p.y+EXIT_START);
		}
		break;
	case ED_TopLeft:
		{
			VEC2 p(float(w/2-road_part)*2+1,(1.f-SPAWN_RATIO)*h*2);
			ep.spawn_rot = 0;
			ep.spawn_area = BOX2D(p.x-SPAWN_RANGE,p.y-SPAWN_RANGE,p.x+SPAWN_RANGE,p.y+SPAWN_RANGE);
			ep.exit_area = BOX2D(p.x-EXIT_WIDTH,p.y+EXIT_START,p.x+EXIT_WIDTH,p.y+EXIT_END);
		}
		break;
	case ED_TopRight:
		{
			VEC2 p(float(w/2+road_part)*2+1,(1.f-SPAWN_RATIO)*h*2);
			ep.spawn_rot = 0;
			ep.spawn_area = BOX2D(p.x-SPAWN_RANGE,p.y-SPAWN_RANGE,p.x+SPAWN_RANGE,p.y+SPAWN_RANGE);
			ep.exit_area = BOX2D(p.x-EXIT_WIDTH,p.y+EXIT_START,p.x+EXIT_WIDTH,p.y+EXIT_END);
		}
		break;
	}

	ep.exit_y = 0;
}

//=================================================================================================
void CityGenerator::CleanBorders()
{
	// bottom / top
	for(int x=1; x<w; ++x)
	{
		height[x] = height[x+(w+1)];
		height[x+h*(w+1)] = height[x+(h-1)*(w+1)];
	}

	// left / right
	for(int y=1; y<h; ++y)
	{
		height[y*(w+1)] = height[y*(w+1)+1];
		height[(y+1)*(w+1)-1] = height[(y+1)*(w+1)-2];
	}

	// corners
	height[0] = (height[1]+height[w+1])/2;
	height[w] = (height[w-1]+height[(w+1)*2-1])/2;
	height[h*(w+1)] = (height[1+h*(w+1)]+height[(h-1)*(w+1)])/2;
	height[(h+1)*(w+1)-1] = (height[(h+1)*(w+1)-2]+height[h*(w+1)-1])/2;
}

//=================================================================================================
void CityGenerator::FlattenRoadExits()
{
	// left
	for(int yy=0; yy<h; ++yy)
	{
		if(tiles[15+yy*w].mode == TM_ROAD)
		{
			float th = height[15+yy*(w+1)];
			for(int y=0; y<=h; ++y)
			{
				for(int x=0; x<15; ++x)
				{
					if(IsPointNearRoad(x, y))
						height[x + y*(w + 1)] = th;
				}
			}
			break;
		}
	}

	// right
	for(int yy=0; yy<h; ++yy)
	{
		if(tiles[w-15+yy*w].mode == TM_ROAD)
		{
			float th = height[w-15+yy*(w+1)];
			for(int y=0; y<=h; ++y)
			{
				for(int x=w-15; x<w; ++x)
				{
					if(IsPointNearRoad(x, y))
						height[x + y*(w + 1)] = th;
				}
			}
			break;
		}
	}

	// bottom
	for(int xx=0; xx<w; ++xx)
	{
		if(tiles[xx+15*w].mode == TM_ROAD)
		{
			float th = height[xx+15*(w+1)];
			for(int y=0; y<15; ++y)
			{
				for(int x=0; x<=w; ++x)
				{
					if(IsPointNearRoad(x, y))
						height[x + y*(w + 1)] = th;
				}
			}
			break;
		}
	}

	// top
	for(int xx=0; xx<w; ++xx)
	{
		if(tiles[xx+(h-15)*w].mode == TM_ROAD)
		{
			float th = height[xx+(h-15)*(w+1)];
			for(int y=h-15; y<=h; ++y)
			{
				for(int x=0; x<=w; ++x)
				{
					if(IsPointNearRoad(x, y))
						height[x + y*(w + 1)] = th;
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
	const int ymax = int(0.75f*w)-5;

	for(int i=0; i<50; ++i)
	{
		INT2 pt(random(ymin,ymax), random(ymin,ymax));
		if(tiles[pt.x+pt.y*w].mode != TM_NORMAL)
			continue;

		int fw = random(4,8);
		int fh = random(4,8);
		if(fw > fh)
			fw *= 2;
		else
			fh *= 2;

		for(int y=pt.y-1; y<=pt.y+fh; ++y)
		{
			for(int x=pt.x-1; x<=pt.x+fw; ++x)
			{
				if(tiles[x+y*w].mode != TM_NORMAL)
					goto next;
			}
		}

		for(int y=pt.y; y<pt.y+fh; ++y)
		{
			for(int x=pt.x; x<pt.x+fw; ++x)
			{
				tiles[x+y*w].Set(TT_FIELD, TM_FIELD);
				float sum = (height[x+y*(w+1)] + height[x+y*(w+1)] + height[x+(y+1)*(w+1)] + height[x+1+(y-1)*(w+1)] + height[x+1+(y+1)*(w+1)])/5;
				height[x+y*(w+1)] = sum;
				height[x+y*(w+1)] = sum;
				height[x+(y+1)*(w+1)] = sum;
				height[x+1+(y-1)*(w+1)] = sum;
				height[x+1+(y+1)*(w+1)] = sum;
			}
		}

next:	;
	}
}

//=================================================================================================
void CityGenerator::ApplyWallTiles(int gates)
{
	const int mur1 = int(0.15f*w);
	const int mur2 = int(0.85f*w);
	const int w1 = w+1;

	// tiles under walls
	for(int i=mur1; i<=mur2; ++i)
	{
		// north
		tiles[i+mur1*w].Set(TT_SAND, TM_BUILDING);
		if(tiles[i+(mur1+1)*w].t == TT_GRASS)
			tiles[i+(mur1+1)*w].Set(TT_SAND, TT_GRASS, 128, TM_BUILDING);
		height[i+mur1*w1] = 1.f;
		height[i+(mur1+1)*w1] = 1.f;
		// south
		tiles[i+mur2*w].Set(TT_SAND, TM_BUILDING);
		if(tiles[i+(mur2-1)*w].t == TT_GRASS)
			tiles[i+(mur2-1)*w].Set(TT_SAND, TT_GRASS, 128, TM_BUILDING);
		height[i+mur2*w1] = 1.f;
		height[i+(mur2-1)*w1] = 1.f;
		// west
		tiles[mur1+i*w].Set(TT_SAND, TM_BUILDING);
		if(tiles[mur1+1+i*w].t == TT_GRASS)
			tiles[mur1+1+i*w].Set(TT_SAND, TT_GRASS, 128, TM_BUILDING);
		height[mur1+i*w1] = 1.f;
		height[mur1+1+i*w1] = 1.f;
		// east
		tiles[mur2+i*w].Set(TT_SAND, TM_BUILDING);
		if(tiles[mur2-1+i*w].t == TT_GRASS)
			tiles[mur2-1+i*w].Set(TT_SAND, TT_GRASS, 128, TM_BUILDING);
		height[mur2+i*w1] = 1.f;
		height[mur2-1+i*w1] = 1.f;
	}

	// tiles under gates
	if(IS_SET(gates, GATE_SOUTH))
	{
		tiles[w/2-1+int(0.15f*w)*w].Set(TT_ROAD, TM_ROAD);
		tiles[w/2  +int(0.15f*w)*w].Set(TT_ROAD, TM_ROAD);
		tiles[w/2+1+int(0.15f*w)*w].Set(TT_ROAD, TM_ROAD);
		tiles[w/2-1+(int(0.15f*w)+1)*w].Set(TT_ROAD, TM_ROAD);
		tiles[w/2  +(int(0.15f*w)+1)*w].Set(TT_ROAD, TM_ROAD);
		tiles[w/2+1+(int(0.15f*w)+1)*w].Set(TT_ROAD, TM_ROAD);
	}
	if(IS_SET(gates, GATE_WEST))
	{
		tiles[int(0.15f*w)+(w/2-1)*w].Set(TT_ROAD, TM_ROAD);
		tiles[int(0.15f*w)+(w/2  )*w].Set(TT_ROAD, TM_ROAD);
		tiles[int(0.15f*w)+(w/2+1)*w].Set(TT_ROAD, TM_ROAD);
		tiles[int(0.15f*w)+1+(w/2-1)*w].Set(TT_ROAD, TM_ROAD);
		tiles[int(0.15f*w)+1+(w/2  )*w].Set(TT_ROAD, TM_ROAD);
		tiles[int(0.15f*w)+1+(w/2+1)*w].Set(TT_ROAD, TM_ROAD);
	}
	if(IS_SET(gates, GATE_NORTH))
	{
		tiles[w/2-1+int(0.85f*w)*w].Set(TT_ROAD, TM_ROAD);
		tiles[w/2  +int(0.85f*w)*w].Set(TT_ROAD, TM_ROAD);
		tiles[w/2+1+int(0.85f*w)*w].Set(TT_ROAD, TM_ROAD);
		tiles[w/2-1+(int(0.85f*w)-1)*w].Set(TT_ROAD, TM_ROAD);
		tiles[w/2  +(int(0.85f*w)-1)*w].Set(TT_ROAD, TM_ROAD);
		tiles[w/2+1+(int(0.85f*w)-1)*w].Set(TT_ROAD, TM_ROAD);
	}
	if(IS_SET(gates, GATE_EAST))
	{
		tiles[int(0.85f*w)+(w/2-1)*w].Set(TT_ROAD, TM_ROAD);
		tiles[int(0.85f*w)+(w/2  )*w].Set(TT_ROAD, TM_ROAD);
		tiles[int(0.85f*w)+(w/2+1)*w].Set(TT_ROAD, TM_ROAD);
		tiles[int(0.85f*w)-1+(w/2-1)*w].Set(TT_ROAD, TM_ROAD);
		tiles[int(0.85f*w)-1+(w/2  )*w].Set(TT_ROAD, TM_ROAD);
		tiles[int(0.85f*w)-1+(w/2+1)*w].Set(TT_ROAD, TM_ROAD);
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
	int index = rand2()%choices;
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
	for(int i=0; i<(int)roads.size(); ++i)
		to_check.push_back(i);

	const int ROAD_MIN_X = (int)(CITY_BORDER_MIN*w),
			  ROAD_MAX_X = (int)(CITY_BORDER_MAX*w),
			  ROAD_MIN_Y = (int)(CITY_BORDER_MIN*h),
			  ROAD_MAX_Y = (int)(CITY_BORDER_MAX*h);
	int choice[3];
	int choices;

	for(int i=0; i<tries; ++i)
	{
		if(to_check.empty())
			break;

		int index = random_item_pop(to_check);
		Road2& r = roads[index];

		choices = 0;
		if(!IS_SET(r.flags, ROAD_START_CHECKED))
			choice[choices++] = RT_START;
		if(!IS_SET(r.flags, ROAD_MID_CHECKED))
			choice[choices++] = RT_MID;
		if(!IS_SET(r.flags, ROAD_END_CHECKED))
			choice[choices++] = RT_END;

		const int type = choice[rand2()%choices];

		INT2 pt;
		const bool horizontal = IS_SET(r.flags, ROAD_HORIZONTAL);

		if(type == RT_MID)
		{
			r.flags |= ROAD_MID_CHECKED;
			INT2 rstart = r.start, rend = r.end;
			if(rstart.x < ROAD_MIN_X)
				r.start.x = ROAD_MIN_X;
			if(rstart.y < ROAD_MIN_Y)
				r.start.y = ROAD_MIN_Y;
			if(rend.x > ROAD_MAX_X)
				rend.x = ROAD_MAX_X;
			if(rend.y > ROAD_MAX_Y)
				rend.y = ROAD_MAX_Y;
			INT2 dir = rend - rstart;
			float ratio = (random()+random())/2;
			if(dir.x)
				pt = INT2(rstart.x + ROAD_MIN_DIST + (dir.x - ROAD_MIN_DIST*2) * ratio, rstart.y);
			else
				pt = INT2(rstart.x, rstart.y + ROAD_MIN_DIST + (dir.y - ROAD_MIN_DIST*2) * ratio);

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

		GAME_DIR dir = (GAME_DIR)get_choice_pop(choice, choices);
		const bool all_done = IS_ALL_SET(r.flags, ROAD_ALL_CHECKED);
		bool try_dual;
		if(MakeAndFillRoad(pt, dir, index))
			try_dual = ((rand2() % ROAD_DUAL_CHANCE) == 0);
		else
			try_dual = ((rand2() % ROAD_DUAL_CHANCE_IF_FAIL) == 0);

		if(try_dual)
		{
			dir = (GAME_DIR)get_choice_pop(choice, choices);
			MakeAndFillRoad(pt, dir, index);
			if(choices && (rand2() % ROAD_TRIPLE_CHANCE) == 0)
				MakeAndFillRoad(pt, (GAME_DIR)choice[0], index);
		}

		if(!all_done)
			to_check.push_back(index);
	}

	to_check.clear();
}

//=================================================================================================
int CityGenerator::MakeRoad(const INT2& start_pt, GAME_DIR dir, int road_index, int& collided_road)
{
	collided_road = -1;

	INT2 pt = start_pt, prev_pt;
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
		INT2 imod;
		if(dir == GDIR_LEFT || dir == GDIR_RIGHT)
			imod = INT2(0, 1);
		else
			imod = INT2(1, 0);
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
void CityGenerator::FillRoad(const INT2& pt, GAME_DIR dir, int dist)
{
	int index = (int)roads.size();
	Road2& road = Add1(roads);
	INT2 start_pt = pt, end_pt = pt;
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

	if(IS_SET(road.flags, ROAD_HORIZONTAL))
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

	for(int y=miny; y<=maxy; ++y)
	{
		for(int x=minx; x<= maxx; ++x)
		{
			int j = x+y*w;
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
bool CityGenerator::MakeAndFillRoad(const INT2& pt, GAME_DIR dir, int road_index)
{
	int collided_road;
	int road_dist = MakeRoad(pt, dir, road_index, collided_road);
	if(collided_road != -1)
	{
		if(rand2()%ROAD_JOIN_CHANCE == 0)
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
			road_dist = (random2(ROAD_MIN_DIST, road_dist) + random2(ROAD_MIN_DIST, road_dist)) / 2;
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
	for(int y=0; y<h; ++y)
	{
		for(int x=0; x<w; ++x)
		{
			TerrainTile& tt = tiles[x+y*w];
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
	float* h = new float[(s+1)*(s+1)];
	vector<EntryPoint> points;
	int gates;
	SetRoadSize(3, 32);

	for(int i = 0; i<(int)RoadType_Max; ++i)
	{
		for(int j = 0; j<4; ++j)
		{
			for(int k = 0; k<4; ++k)
			{
				for(int l = 0; l<swaps[i]; ++l)
				{
					for(int m = 0; m<2; ++m)
					{
						LOG(Format("Test road %d %d %d %d %d", i, j, k, l, m));
						Init(tiles, h, s, s);
						GenerateMainRoad((RoadType)i, (GAME_DIR)j, k, m == 0 ? false : true, l, points, gates, false);
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
		if(tiles[x - 1 + y*w].mode == TM_ROAD)
			return true;
	}

	if(x != w && y != h)
	{
		if(tiles[x + y*w].mode == TM_ROAD)
			return true;
	}

	return false;
}
