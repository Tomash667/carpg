#include "Pch.h"
#include "GameCore.h"
#include "Pathfinding.h"
#include "Level.h"
#include "OutsideLocation.h"
#include "InsideLocation.h"
#include "Game.h"
#include "Terrain.h"
#include "DebugDrawer.h"

//-----------------------------------------------------------------------------
const float SS = 0.25f;//0.25f/8;


//-----------------------------------------------------------------------------
struct Point
{
	Int2 pt, prev;
};

//-----------------------------------------------------------------------------
struct AStarSort
{
	AStarSort(vector<Pathfinding::APoint>& a_map, int rozmiar) : a_map(a_map), rozmiar(rozmiar)
	{
	}

	bool operator() (const Point& pt1, const Point& pt2) const
	{
		return a_map[pt1.pt.x + pt1.pt.y*rozmiar].suma > a_map[pt2.pt.x + pt2.pt.y*rozmiar].suma;
	}

	vector<Pathfinding::APoint>& a_map;
	int rozmiar;
};


//=================================================================================================
// szuka œcie¿ki u¿ywaj¹c algorytmu A-Star
// zwraca true jeœli znaleziono i w wektorze jest ta œcie¿ka, w œcie¿ce nie ma pocz¹tkowego kafelka
bool Pathfinding::FindPath(LevelContext& ctx, const Int2& _start_tile, const Int2& _target_tile, vector<Int2>& _path, bool can_open_doors, bool wedrowanie, vector<Int2>* blocked)
{
	if(_start_tile == _target_tile || ctx.type == LevelContext::Building)
	{
		// jest w tym samym miejscu
		_path.clear();
		_path.push_back(_target_tile);
		return true;
	}

	static vector<Point> do_sprawdzenia;
	do_sprawdzenia.clear();

	if(ctx.type == LevelContext::Outside)
	{
		// zewnêtrze
		OutsideLocation* outside = (OutsideLocation*)L.location;
		const TerrainTile* m = outside->tiles;
		const int w = OutsideLocation::size;

		// czy poza map¹
		if(!outside->IsInside(_start_tile) || !outside->IsInside(_target_tile))
			return false;

		// czy na blokuj¹cym miejscu
		if(m[_start_tile(w)].IsBlocking() || m[_target_tile(w)].IsBlocking())
			return false;

		// powiêksz mapê
		const uint size = uint(w*w);
		if(size > a_map.size())
			a_map.resize(size);

		// wyzeruj
		memset(&a_map[0], 0, sizeof(APoint)*size);
		_path.clear();

		// apply blocked tiles
		if(blocked)
		{
			for(vector<Int2>::iterator it = blocked->begin(), end = blocked->end(); it != end; ++it)
				a_map[(*it)(w)].stan = 1;
			if(a_map[_start_tile(w)].stan == 1 || a_map[_target_tile(w)].stan == 1)
				return false;
		}

		// dodaj pierwszy kafelek do sprawdzenia
		APoint apt, prev_apt;
		apt.suma = apt.odleglosc = 10 * (abs(_target_tile.x - _start_tile.x) + abs(_target_tile.y - _start_tile.y));
		apt.stan = 1;
		apt.koszt = 0;
		a_map[_start_tile(w)] = apt;

		Point pt, new_pt;
		pt.pt = pt.prev = _start_tile;
		do_sprawdzenia.push_back(pt);

		AStarSort sortownik(a_map, w);

		// szukaj drogi
		while(!do_sprawdzenia.empty())
		{
			pt = do_sprawdzenia.back();
			do_sprawdzenia.pop_back();

			apt = a_map[pt.pt(w)];
			prev_apt = a_map[pt.prev(w)];

			if(pt.pt == _target_tile)
			{
				APoint& ept = a_map[pt.pt(w)];
				ept.stan = 1;
				ept.prev = pt.prev;
				break;
			}

			const Int2 kierunek[4] = {
				Int2(0,1),
				Int2(1,0),
				Int2(0,-1),
				Int2(-1,0)
			};

			const Int2 kierunek2[4] = {
				Int2(1,1),
				Int2(1,-1),
				Int2(-1,1),
				Int2(-1,-1)
			};

			for(int i = 0; i < 4; ++i)
			{
				const Int2& pt1 = kierunek[i] + pt.pt;
				const Int2& pt2 = kierunek2[i] + pt.pt;

				if(pt1.x >= 0 && pt1.y >= 0 && pt1.x < w - 1 && pt1.y < w - 1 && a_map[pt1(w)].stan == 0 && !m[pt1(w)].IsBlocking())
				{
					apt.prev = pt.pt;
					apt.koszt = prev_apt.koszt + 10;
					if(wedrowanie)
					{
						TERRAIN_TILE type = m[pt1(w)].t;
						if(type == TT_SAND)
							apt.koszt += 10;
						else if(type != TT_ROAD)
							apt.koszt += 30;
					}
					apt.odleglosc = (abs(pt1.x - _target_tile.x) + abs(pt1.y - _target_tile.y)) * 10;
					apt.suma = apt.odleglosc + apt.koszt;
					apt.stan = 1;
					a_map[pt1(w)] = apt;

					new_pt.pt = pt1;
					new_pt.prev = pt.pt;
					do_sprawdzenia.push_back(new_pt);
				}

				if(pt2.x >= 0 && pt2.y >= 0 && pt2.x < w - 1 && pt2.y < w - 1 && a_map[pt2(w)].stan == 0 &&
					!m[pt2(w)].IsBlocking() &&
					!m[kierunek2[i].x + pt.pt.x + pt.pt.y*w].IsBlocking() &&
					!m[pt.pt.x + (kierunek2[i].y + pt.pt.y)*w].IsBlocking())
				{
					apt.prev = pt.pt;
					apt.koszt = prev_apt.koszt + 15;
					if(wedrowanie)
					{
						TERRAIN_TILE type = m[pt2(w)].t;
						if(type == TT_SAND)
							apt.koszt += 10;
						else if(type != TT_ROAD)
							apt.koszt += 30;
					}
					apt.odleglosc = (abs(pt2.x - _target_tile.x) + abs(pt2.y - _target_tile.y)) * 10;
					apt.suma = apt.odleglosc + apt.koszt;
					apt.stan = 1;
					a_map[pt2(w)] = apt;

					new_pt.pt = pt2;
					new_pt.prev = pt.pt;
					do_sprawdzenia.push_back(new_pt);
				}
			}

			std::sort(do_sprawdzenia.begin(), do_sprawdzenia.end(), sortownik);
		}

		// jeœli cel jest niezbadany to nie znaleziono œcie¿ki
		if(a_map[_target_tile(w)].stan == 0)
			return false;

		// wype³nij elementami œcie¿kê
		_path.push_back(_target_tile);

		Int2 p;

		while((p = _path.back()) != _start_tile)
			_path.push_back(a_map[p(w)].prev);
	}
	else
	{
		// wnêtrze
		InsideLocation* inside = (InsideLocation*)L.location;
		InsideLocationLevel& lvl = inside->GetLevelData();
		const Tile* m = lvl.map;
		const int w = lvl.w, h = lvl.h;

		// czy poza map¹
		if(!lvl.IsInside(_start_tile) || !lvl.IsInside(_target_tile))
			return false;

		// czy na blokuj¹cym miejscu
		if(IsBlocking(m[_start_tile(w)]) || IsBlocking(m[_target_tile(w)]))
			return false;

		// powiêksz mapê
		const uint size = uint(w*h);
		if(size > a_map.size())
			a_map.resize(size);

		// wyzeruj
		memset(&a_map[0], 0, sizeof(APoint)*size);
		_path.clear();

		// apply blocked tiles
		if(blocked)
		{
			for(vector<Int2>::iterator it = blocked->begin(), end = blocked->end(); it != end; ++it)
				a_map[(*it)(w)].stan = 1;
			if(a_map[_start_tile(w)].stan == 1 || a_map[_target_tile(w)].stan == 1)
				return false;
		}

		// dodaj pierwszy kafelek do sprawdzenia
		APoint apt, prev_apt;
		apt.suma = apt.odleglosc = 10 * (abs(_target_tile.x - _start_tile.x) + abs(_target_tile.y - _start_tile.y));
		apt.stan = 1;
		apt.koszt = 0;
		a_map[_start_tile(w)] = apt;

		Point pt, new_pt;
		pt.pt = pt.prev = _start_tile;
		do_sprawdzenia.push_back(pt);

		AStarSort sortownik(a_map, w);

		// szukaj drogi
		while(!do_sprawdzenia.empty())
		{
			pt = do_sprawdzenia.back();
			do_sprawdzenia.pop_back();

			prev_apt = a_map[pt.pt(w)];

			//Info("(%d,%d)->(%d,%d), K:%d D:%d S:%d", pt.prev.x, pt.prev.y, pt.pt.x, pt.pt.y, prev_apt.koszt, prev_apt.odleglosc, prev_apt.suma);

			if(pt.pt == _target_tile)
			{
				APoint& ept = a_map[pt.pt(w)];
				ept.stan = 1;
				ept.prev = pt.prev;
				break;
			}

			const Int2 kierunek[4] = {
				Int2(0,1),
				Int2(1,0),
				Int2(0,-1),
				Int2(-1,0)
			};

			const Int2 kierunek2[4] = {
				Int2(1,1),
				Int2(1,-1),
				Int2(-1,1),
				Int2(-1,-1)
			};

			if(can_open_doors)
			{
				for(int i = 0; i < 4; ++i)
				{
					const Int2& pt1 = kierunek[i] + pt.pt;
					const Int2& pt2 = kierunek2[i] + pt.pt;

					if(pt1.x >= 0 && pt1.y >= 0 && pt1.x < w - 1 && pt1.y < h - 1 && !IsBlocking(m[pt1(w)]))
					{
						apt.prev = pt.pt;
						apt.koszt = prev_apt.koszt + 10;
						apt.odleglosc = (abs(pt1.x - _target_tile.x) + abs(pt1.y - _target_tile.y)) * 10;
						apt.suma = apt.odleglosc + apt.koszt;

						if(a_map[pt1(w)].IsLower(apt.suma))
						{
							apt.stan = 1;
							a_map[pt1(w)] = apt;

							new_pt.pt = pt1;
							new_pt.prev = pt.pt;
							do_sprawdzenia.push_back(new_pt);
						}
					}

					if(pt2.x >= 0 && pt2.y >= 0 && pt2.x < w - 1 && pt2.y < h - 1 &&
						!IsBlocking(m[pt2(w)]) &&
						!IsBlocking(m[kierunek2[i].x + pt.pt.x + pt.pt.y*w]) &&
						!IsBlocking(m[pt.pt.x + (kierunek2[i].y + pt.pt.y)*w]))
					{
						apt.prev = pt.pt;
						apt.koszt = prev_apt.koszt + 15;
						apt.odleglosc = (abs(pt2.x - _target_tile.x) + abs(pt2.y - _target_tile.y)) * 10;
						apt.suma = apt.odleglosc + apt.koszt;

						if(a_map[pt2(w)].IsLower(apt.suma))
						{
							apt.stan = 1;
							a_map[pt2(w)] = apt;

							new_pt.pt = pt2;
							new_pt.prev = pt.pt;
							do_sprawdzenia.push_back(new_pt);
						}
					}
				}
			}
			else
			{
				for(int i = 0; i < 4; ++i)
				{
					const Int2& pt1 = kierunek[i] + pt.pt;
					const Int2& pt2 = kierunek2[i] + pt.pt;

					if(pt1.x >= 0 && pt1.y >= 0 && pt1.x < w - 1 && pt1.y < h - 1 && !IsBlocking(m[pt1(w)]))
					{
						int ok = 2; // 2-ok i dodaj, 1-ok, 0-nie

						if(m[pt1(w)].type == DOORS)
						{
							Door* door = L.FindDoor(ctx, pt1);
							if(door && door->IsBlocking())
							{
								// ustal gdzie s¹ drzwi na polu i czy z tej strony mo¿na na nie wejœæ
								if(IsBlocking(lvl.map[pt1.x - 1 + pt1.y*lvl.w].type))
								{
									// #   #
									// #---#
									// #   #
									int mov = 0;
									if(lvl.rooms[lvl.map[pt1.x + (pt1.y - 1)*lvl.w].room]->IsCorridor())
										++mov;
									if(lvl.rooms[lvl.map[pt1.x + (pt1.y + 1)*lvl.w].room]->IsCorridor())
										--mov;
									if(mov == 1)
									{
										// #---#
										// #   #
										// #   #
										if(i != 0)
											ok = 0;
									}
									else if(mov == -1)
									{
										// #   #
										// #   #
										// #---#
										if(i != 2)
											ok = 0;
									}
									else
										ok = 1;
								}
								else
								{
									// ###
									//  |
									//  |
									// ###
									int mov = 0;
									if(lvl.rooms[lvl.map[pt1.x - 1 + pt1.y*lvl.w].room]->IsCorridor())
										++mov;
									if(lvl.rooms[lvl.map[pt1.x + 1 + pt1.y*lvl.w].room]->IsCorridor())
										--mov;
									if(mov == 1)
									{
										// ###
										//   |
										//   |
										// ###
										if(i != 1)
											ok = 0;
									}
									else if(mov == -1)
									{
										// ###
										// |
										// |
										// ###
										if(i != 3)
											ok = 0;
									}
									else
										ok = 1;
								}
							}
						}

						if(ok)
						{
							apt.prev = pt.pt;
							apt.koszt = prev_apt.koszt + 10;
							apt.odleglosc = (abs(pt1.x - _target_tile.x) + abs(pt1.y - _target_tile.y)) * 10;
							apt.suma = apt.odleglosc + apt.koszt;

							if(a_map[pt1(w)].IsLower(apt.suma))
							{
								apt.stan = 1;
								a_map[pt1(w)] = apt;

								if(ok == 2)
								{
									new_pt.pt = pt1;
									new_pt.prev = pt.pt;
									do_sprawdzenia.push_back(new_pt);
								}
							}
						}
					}

					if(pt2.x >= 0 && pt2.y >= 0 && pt2.x < w - 1 && pt2.y < h - 1 &&
						!IsBlocking(m[pt2(w)]) &&
						!IsBlocking(m[kierunek2[i].x + pt.pt.x + pt.pt.y*w]) &&
						!IsBlocking(m[pt.pt.x + (kierunek2[i].y + pt.pt.y)*w]))
					{
						bool ok = true;

						if(m[pt2(w)].type == DOORS)
						{
							Door* door = L.FindDoor(ctx, pt2);
							if(door && door->IsBlocking())
								ok = false;
						}

						if(ok && m[kierunek2[i].x + pt.pt.x + pt.pt.y*w].type == DOORS)
						{
							Door* door = L.FindDoor(ctx, Int2(kierunek2[i].x + pt.pt.x, pt.pt.y));
							if(door && door->IsBlocking())
								ok = false;
						}

						if(ok && m[pt.pt.x + (kierunek2[i].y + pt.pt.y)*w].type == DOORS)
						{
							Door* door = L.FindDoor(ctx, Int2(pt.pt.x, kierunek2[i].y + pt.pt.y));
							if(door && door->IsBlocking())
								ok = false;
						}

						if(ok)
						{
							apt.prev = pt.pt;
							apt.koszt = prev_apt.koszt + 15;
							apt.odleglosc = (abs(pt2.x - _target_tile.x) + abs(pt2.y - _target_tile.y)) * 10;
							apt.suma = apt.odleglosc + apt.koszt;

							if(a_map[pt2(w)].IsLower(apt.suma))
							{
								apt.stan = 1;
								a_map[pt2(w)] = apt;

								new_pt.pt = pt2;
								new_pt.prev = pt.pt;
								do_sprawdzenia.push_back(new_pt);
							}
						}
					}
				}
			}

			std::sort(do_sprawdzenia.begin(), do_sprawdzenia.end(), sortownik);
		}

		// jeœli cel jest niezbadany to nie znaleziono œcie¿ki
		if(a_map[_target_tile(w)].stan == 0)
			return false;

		// wype³nij elementami œcie¿kê
		_path.push_back(_target_tile);

		Int2 p;

		while((p = _path.back()) != _start_tile)
			_path.push_back(a_map[p(w)].prev);
	}

	// usuñ ostatni element œcie¿ki
	_path.pop_back();

	return true;
}

//=================================================================================================
// 0 - path found
// 1 - start pos and target pos too far
// 2 - start location is blocked
// 3 - start tile and target tile is equal
// 4 - target tile is blocked
// 5 - path not found
int Pathfinding::FindLocalPath(LevelContext& ctx, vector<Int2>& _path, const Int2& my_tile, const Int2& target_tile, const Unit* _me, const Unit* _other, const void* usable, bool is_end_point)
{
	assert(_me);

	_path.clear();
	if(marked == _me)
		test_pf.clear();

	if(my_tile == target_tile)
		return 3;

	int dist = Int2::Distance(my_tile, target_tile);
	if(dist >= 32)
		return 1;

	// œrodek
	const Int2 center_tile((my_tile + target_tile) / 2);

	int minx = max(ctx.mine.x * 8, center_tile.x - 15),
		miny = max(ctx.mine.y * 8, center_tile.y - 15),
		maxx = min(ctx.maxe.x * 8 - 1, center_tile.x + 16),
		maxy = min(ctx.maxe.y * 8 - 1, center_tile.y + 16);

	int w = maxx - minx,
		h = maxy - miny;

	uint size = (w + 1)*(h + 1);

	if(a_map.size() < size)
		a_map.resize(size);
	memset(&a_map[0], 0, sizeof(APoint)*size);
	if(local_pfmap.size() < size)
		local_pfmap.resize(size);

	const Unit* ignored_units[3] = { _me, 0 };
	if(_other)
		ignored_units[1] = _other;
	Level::IgnoreObjects ignore = { 0 };
	ignore.ignored_units = (const Unit**)ignored_units;
	const void* ignored_objects[2] = { 0 };
	if(usable)
	{
		ignored_objects[0] = usable;
		ignore.ignored_objects = ignored_objects;
	}

	L.global_col.clear();
	L.GatherCollisionObjects(ctx, L.global_col, Box2d(float(minx) / 4 - 0.25f, float(miny) / 4 - 0.25f, float(maxx) / 4 + 0.25f, float(maxy) / 4 + 0.25f), &ignore);

	const float r = _me->GetUnitRadius() - 0.25f / 2;

	if(marked == _me)
	{
		for(int y = miny, y2 = 0; y < maxy; ++y, ++y2)
		{
			for(int x = minx, x2 = 0; x < maxx; ++x, ++x2)
			{
				if(!L.Collide(L.global_col, Box2d(0.25f*x, 0.25f*y, 0.25f*(x + 1), 0.25f*(y + 1)), r))
				{
					test_pf.push_back(pair<Vec2, int>(Vec2(0.25f*x, 0.25f*y), 0));
					local_pfmap[x2 + y2 * w] = false;
				}
				else
					local_pfmap[x2 + y2 * w] = true;
			}
		}
	}
	else
	{
		for(int y = miny, y2 = 0; y < maxy; ++y, ++y2)
		{
			for(int x = minx, x2 = 0; x < maxx; ++x, ++x2)
				local_pfmap[x2 + y2 * w] = L.Collide(L.global_col, Box2d(0.25f*x, 0.25f*y, 0.25f*(x + 1), 0.25f*(y + 1)), r);
		}
	}

	const Int2 target_rel(target_tile.x - minx, target_tile.y - miny),
		my_rel(my_tile.x - minx, my_tile.y - miny);

	// target tile is blocked
	if(!is_end_point && local_pfmap[target_rel(w)])
		return 4;

	// oznacz moje pole jako wolne
	local_pfmap[my_rel(w)] = false;
	const Int2 neis[8] = {
		Int2(-1,-1),
		Int2(0,-1),
		Int2(1,-1),
		Int2(-1,0),
		Int2(1,0),
		Int2(-1,1),
		Int2(0,1),
		Int2(1,1)
	};

	bool jest = false;
	for(int i = 0; i < 8; ++i)
	{
		Int2 pt = my_rel + neis[i];
		if(pt.x < 0 || pt.y < 0 || pt.x > 32 || pt.y > 32)
			continue;
		if(!local_pfmap[pt(w)])
		{
			jest = true;
			break;
		}
	}

	if(!jest)
		return 2;

	// dodaj pierwszy punkt do sprawdzenia
	static vector<Point> do_sprawdzenia;
	do_sprawdzenia.clear();
	{
		Point& pt = Add1(do_sprawdzenia);
		pt.pt = target_rel;
		pt.prev = Int2(0, 0);
	}

	APoint apt, prev_apt;
	apt.odleglosc = apt.suma = Int2::Distance(my_rel, target_rel) * 10;
	apt.koszt = 0;
	apt.stan = 1;
	apt.prev = Int2(0, 0);
	a_map[target_rel(w)] = apt;

	AStarSort sortownik(a_map, w);
	Point new_pt;

	const int MAX_STEPS = 100;
	int steps = 0;

	// rozpocznij szukanie œcie¿ki
	while(!do_sprawdzenia.empty())
	{
		Point pt = do_sprawdzenia.back();
		do_sprawdzenia.pop_back();
		prev_apt = a_map[pt.pt(w)];

		if(pt.pt == my_rel)
		{
			APoint& ept = a_map[pt.pt(w)];
			ept.stan = 1;
			ept.prev = pt.prev;
			break;
		}

		const Int2 kierunek[4] = {
			Int2(0,1),
			Int2(1,0),
			Int2(0,-1),
			Int2(-1,0)
		};

		const Int2 kierunek2[4] = {
			Int2(1,1),
			Int2(1,-1),
			Int2(-1,1),
			Int2(-1,-1)
		};

		for(int i = 0; i < 4; ++i)
		{
			const Int2& pt1 = kierunek[i] + pt.pt;
			const Int2& pt2 = kierunek2[i] + pt.pt;

			if(pt1.x >= 0 && pt1.y >= 0 && pt1.x < w - 1 && pt1.y < h - 1 && !local_pfmap[pt1(w)])
			{
				apt.prev = pt.pt;
				apt.koszt = prev_apt.koszt + 10;
				apt.odleglosc = Int2::Distance(pt1, my_rel) * 10;
				apt.suma = apt.odleglosc + apt.koszt;

				if(a_map[pt1(w)].IsLower(apt.suma))
				{
					apt.stan = 1;
					a_map[pt1(w)] = apt;

					new_pt.pt = pt1;
					new_pt.prev = pt.pt;
					do_sprawdzenia.push_back(new_pt);
				}
			}

			if(pt2.x >= 0 && pt2.y >= 0 && pt2.x < w - 1 && pt2.y < h - 1 && !local_pfmap[pt2(w)] /*&&
				!local_pfmap[kierunek2[i].x+pt.pt.x+pt.pt.y*w] && !local_pfmap[pt.pt.x+(kierunek2[i].y+pt.pt.y)*w]*/)
			{
				apt.prev = pt.pt;
				apt.koszt = prev_apt.koszt + 15;
				apt.odleglosc = Int2::Distance(pt2, my_rel) * 10;
				apt.suma = apt.odleglosc + apt.koszt;

				if(a_map[pt2(w)].IsLower(apt.suma))
				{
					apt.stan = 1;
					a_map[pt2(w)] = apt;

					new_pt.pt = pt2;
					new_pt.prev = pt.pt;
					do_sprawdzenia.push_back(new_pt);
				}
			}
		}

		++steps;
		if(steps > MAX_STEPS)
			break;

		std::sort(do_sprawdzenia.begin(), do_sprawdzenia.end(), sortownik);
	}

	if(marked)
	{
		if(marked == _me)
			test_pf_outside = (L.location->outside && _me->in_building == -1);

		if(a_map[my_rel(w)].stan == 0)
		{
			if(marked == _me)
			{
				test_pf.push_back(pair<Vec2, int>(Vec2(0.25f*my_tile.x, 0.25f*my_tile.y), 1));
				test_pf.push_back(pair<Vec2, int>(Vec2(0.25f*target_tile.x, 0.25f*target_tile.y), 1));
			}
			return 5;
		}

		if(marked == _me)
		{
			test_pf.push_back(pair<Vec2, int>(Vec2(0.25f*my_tile.x, 0.25f*my_tile.y), 1));
			test_pf.push_back(pair<Vec2, int>(Vec2(0.25f*target_tile.x, 0.25f*target_tile.y), 1));
		}

		// populate path vector (and debug drawing)
		Int2 p = my_rel;

		do
		{
			if(marked == _me)
				test_pf.push_back(pair<Vec2, int>(Vec2(0.25f*(p.x + minx), 0.25f*(p.y + miny)), 1));
			_path.push_back(Int2(p.x + minx, p.y + miny));
			p = a_map[p(w)].prev;
		} while(p != target_rel);
	}
	else
	{
		if(a_map[my_rel(w)].stan == 0)
			return 5;

		// populate path vector (and debug drawing)
		Int2 p = my_rel;

		do
		{
			_path.push_back(Int2(p.x + minx, p.y + miny));
			p = a_map[p(w)].prev;
		} while(p != target_rel);
	}

	_path.push_back(target_tile);
	std::reverse(_path.begin(), _path.end());
	_path.pop_back();

	return 0;
}

//=================================================================================================
void Pathfinding::Draw(DebugDrawer* dd)
{
	if(test_pf.empty() || !marked)
		return;

	dd->BeginBatch();

	for(vector<pair<Vec2, int>>::iterator it = test_pf.begin(), end = test_pf.end(); it != end; ++it)
	{
		Vec3 v[4] = {
			Vec3(it->first.x, 0.1f, it->first.y + SS),
			Vec3(it->first.x + SS, 0.1f, it->first.y + SS),
			Vec3(it->first.x, 0.1f, it->first.y),
			Vec3(it->first.x + SS, 0.1f, it->first.y)
		};

		if(test_pf_outside)
		{
			float h = L.terrain->GetH(v[0].x, v[0].z) + 0.1f;
			for(int i = 0; i < 4; ++i)
				v[i].y = h;
		}

		Vec4 color;
		switch(it->second)
		{
		case 0:
			color = Vec4(0, 1, 0, 0.5f);
			break;
		case 1:
			color = Vec4(1, 0, 0, 0.5f);
			break;
		case 2:
			color = Vec4(0, 0, 0, 0.5f);
			break;
		}

		dd->AddQuad(v, color);
	}

	dd->EndBatch();
}
