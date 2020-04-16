#include "Pch.h"
#include "Pathfinding.h"
#include "Level.h"
#include "OutsideLocation.h"
#include "InsideLocation.h"
#include "Terrain.h"
#include "BasicShader.h"

//-----------------------------------------------------------------------------
const float SS = 0.25f;
Pathfinding* global::pathfinding;

//-----------------------------------------------------------------------------
struct Point
{
	Int2 pt, prev;
};

//-----------------------------------------------------------------------------
struct AStarSort
{
	AStarSort(vector<Pathfinding::APoint>& a_map, int size) : a_map(a_map), size(size)
	{
	}

	bool operator() (const Point& pt1, const Point& pt2) const
	{
		return a_map[pt1.pt.x + pt1.pt.y*size].sum > a_map[pt2.pt.x + pt2.pt.y*size].sum;
	}

	vector<Pathfinding::APoint>& a_map;
	int size;
};

//-----------------------------------------------------------------------------
const Int2 c_dir[4] = {
	Int2(0,1),
	Int2(1,0),
	Int2(0,-1),
	Int2(-1,0)
};

const Int2 c_dir2[4] = {
	Int2(1,1),
	Int2(1,-1),
	Int2(-1,1),
	Int2(-1,-1)
};

//=================================================================================================
// Search for path using A-Star algorithm
// Return true if there is path, retured path don't contain first tile
bool Pathfinding::FindPath(LevelArea& area, const Int2& start_tile, const Int2& target_tile, vector<Int2>& path, bool can_open_doors, bool wandering, vector<Int2>* blocked)
{
	if(start_tile == target_tile || area.area_type == LevelArea::Type::Building)
	{
		// already at same tile
		path.clear();
		path.push_back(target_tile);
		return true;
	}

	static vector<Point> to_check;
	to_check.clear();

	if(area.area_type == LevelArea::Type::Outside)
	{
		// outside location
		OutsideLocation* outside = static_cast<OutsideLocation*>(game_level->location);
		const TerrainTile* m = outside->tiles;
		const int w = OutsideLocation::size;

		// is inside map?
		if(!outside->IsInside(start_tile) || !outside->IsInside(target_tile))
			return false;

		// is in blocking tile?
		if(m[start_tile(w)].IsBlocking() || m[target_tile(w)].IsBlocking())
			return false;

		// resize map
		const uint size = uint(w*w);
		if(size > a_map.size())
			a_map.resize(size);

		// zero out
		memset(&a_map[0], 0, sizeof(APoint)*size);
		path.clear();

		// apply blocked tiles
		if(blocked)
		{
			for(vector<Int2>::iterator it = blocked->begin(), end = blocked->end(); it != end; ++it)
				a_map[(*it)(w)].state = 1;
			if(a_map[start_tile(w)].state == 1 || a_map[target_tile(w)].state == 1)
				return false;
		}

		// add first tile to check
		APoint apt, prev_apt;
		apt.sum = apt.dist = 10 * (abs(target_tile.x - start_tile.x) + abs(target_tile.y - start_tile.y));
		apt.state = 1;
		apt.cost = 0;
		a_map[start_tile(w)] = apt;

		Point pt, new_pt;
		pt.pt = pt.prev = start_tile;
		to_check.push_back(pt);

		AStarSort sorter(a_map, w);

		// search for path
		while(!to_check.empty())
		{
			pt = to_check.back();
			to_check.pop_back();

			apt = a_map[pt.pt(w)];
			prev_apt = a_map[pt.prev(w)];

			if(pt.pt == target_tile)
			{
				APoint& ept = a_map[pt.pt(w)];
				ept.state = 1;
				ept.prev = pt.prev;
				break;
			}

			for(int i = 0; i < 4; ++i)
			{
				const Int2& pt1 = c_dir[i] + pt.pt;
				const Int2& pt2 = c_dir2[i] + pt.pt;

				if(pt1.x >= 0 && pt1.y >= 0 && pt1.x < w - 1 && pt1.y < w - 1 && a_map[pt1(w)].state == 0 && !m[pt1(w)].IsBlocking())
				{
					apt.prev = pt.pt;
					apt.cost = prev_apt.cost + 10;
					if(wandering)
					{
						TERRAIN_TILE type = m[pt1(w)].t;
						if(type == TT_SAND)
							apt.cost += 10;
						else if(type != TT_ROAD)
							apt.cost += 30;
					}
					apt.dist = (abs(pt1.x - target_tile.x) + abs(pt1.y - target_tile.y)) * 10;
					apt.sum = apt.dist + apt.cost;
					apt.state = 1;
					a_map[pt1(w)] = apt;

					new_pt.pt = pt1;
					new_pt.prev = pt.pt;
					to_check.push_back(new_pt);
				}

				if(pt2.x >= 0 && pt2.y >= 0 && pt2.x < w - 1 && pt2.y < w - 1 && a_map[pt2(w)].state == 0
					&& !m[pt2(w)].IsBlocking()
					&& !m[c_dir2[i].x + pt.pt.x + pt.pt.y*w].IsBlocking()
					&& !m[pt.pt.x + (c_dir2[i].y + pt.pt.y)*w].IsBlocking())
				{
					apt.prev = pt.pt;
					apt.cost = prev_apt.cost + 15;
					if(wandering)
					{
						TERRAIN_TILE type = m[pt2(w)].t;
						if(type == TT_SAND)
							apt.cost += 10;
						else if(type != TT_ROAD)
							apt.cost += 30;
					}
					apt.dist = (abs(pt2.x - target_tile.x) + abs(pt2.y - target_tile.y)) * 10;
					apt.sum = apt.dist + apt.cost;
					apt.state = 1;
					a_map[pt2(w)] = apt;

					new_pt.pt = pt2;
					new_pt.prev = pt.pt;
					to_check.push_back(new_pt);
				}
			}

			std::sort(to_check.begin(), to_check.end(), sorter);
		}

		// if target is not visited then path was not found
		if(a_map[target_tile(w)].state == 0)
			return false;

		// fill path elements
		path.push_back(target_tile);

		Int2 p;

		while((p = path.back()) != start_tile)
			path.push_back(a_map[p(w)].prev);
	}
	else
	{
		// inside location
		InsideLocation* inside = static_cast<InsideLocation*>(game_level->location);
		InsideLocationLevel& lvl = inside->GetLevelData();
		const Tile* m = lvl.map;
		const int w = lvl.w, h = lvl.h;

		// is inside level?
		if(!lvl.IsInside(start_tile) || !lvl.IsInside(target_tile))
			return false;

		// is in blocking tile?
		if(IsBlocking(m[start_tile(w)]) || IsBlocking(m[target_tile(w)]))
			return false;

		// resize map
		const uint size = uint(w*h);
		if(size > a_map.size())
			a_map.resize(size);

		// zero out
		memset(&a_map[0], 0, sizeof(APoint)*size);
		path.clear();

		// apply blocked tiles
		if(blocked)
		{
			for(vector<Int2>::iterator it = blocked->begin(), end = blocked->end(); it != end; ++it)
				a_map[(*it)(w)].state = 1;
			if(a_map[start_tile(w)].state == 1 || a_map[target_tile(w)].state == 1)
				return false;
		}

		// add first tile to check
		APoint apt, prev_apt;
		apt.sum = apt.dist = 10 * (abs(target_tile.x - start_tile.x) + abs(target_tile.y - start_tile.y));
		apt.state = 1;
		apt.cost = 0;
		a_map[start_tile(w)] = apt;

		Point pt, new_pt;
		pt.pt = pt.prev = start_tile;
		to_check.push_back(pt);

		AStarSort sorter(a_map, w);

		// search for path
		while(!to_check.empty())
		{
			pt = to_check.back();
			to_check.pop_back();
			prev_apt = a_map[pt.pt(w)];

			if(pt.pt == target_tile)
			{
				APoint& ept = a_map[pt.pt(w)];
				ept.state = 1;
				ept.prev = pt.prev;
				break;
			}

			if(can_open_doors)
			{
				for(int i = 0; i < 4; ++i)
				{
					const Int2& pt1 = c_dir[i] + pt.pt;
					const Int2& pt2 = c_dir2[i] + pt.pt;

					if(pt1.x >= 0 && pt1.y >= 0 && pt1.x < w - 1 && pt1.y < h - 1 && !IsBlocking(m[pt1(w)]))
					{
						apt.prev = pt.pt;
						apt.cost = prev_apt.cost + 10;
						apt.dist = (abs(pt1.x - target_tile.x) + abs(pt1.y - target_tile.y)) * 10;
						apt.sum = apt.dist + apt.cost;

						if(a_map[pt1(w)].IsLower(apt.sum))
						{
							apt.state = 1;
							a_map[pt1(w)] = apt;

							new_pt.pt = pt1;
							new_pt.prev = pt.pt;
							to_check.push_back(new_pt);
						}
					}

					if(pt2.x >= 0 && pt2.y >= 0 && pt2.x < w - 1 && pt2.y < h - 1
						&& !IsBlocking(m[pt2(w)])
						&& !IsBlocking(m[c_dir2[i].x + pt.pt.x + pt.pt.y*w])
						&& !IsBlocking(m[pt.pt.x + (c_dir2[i].y + pt.pt.y)*w]))
					{
						apt.prev = pt.pt;
						apt.cost = prev_apt.cost + 15;
						apt.dist = (abs(pt2.x - target_tile.x) + abs(pt2.y - target_tile.y)) * 10;
						apt.sum = apt.dist + apt.cost;

						if(a_map[pt2(w)].IsLower(apt.sum))
						{
							apt.state = 1;
							a_map[pt2(w)] = apt;

							new_pt.pt = pt2;
							new_pt.prev = pt.pt;
							to_check.push_back(new_pt);
						}
					}
				}
			}
			else
			{
				for(int i = 0; i < 4; ++i)
				{
					const Int2& pt1 = c_dir[i] + pt.pt;
					const Int2& pt2 = c_dir2[i] + pt.pt;

					if(pt1.x >= 0 && pt1.y >= 0 && pt1.x < w - 1 && pt1.y < h - 1 && !IsBlocking(m[pt1(w)]))
					{
						int ok = 2; // 2-ok & next, 1-ok, 0-no

						if(m[pt1(w)].type == DOORS)
						{
							Door* door = area.FindDoor(pt1);
							if(door && door->IsBlocking())
							{
								// decide if door can be entered from this tile
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
							apt.cost = prev_apt.cost + 10;
							apt.dist = (abs(pt1.x - target_tile.x) + abs(pt1.y - target_tile.y)) * 10;
							apt.sum = apt.dist + apt.cost;

							if(a_map[pt1(w)].IsLower(apt.sum))
							{
								apt.state = 1;
								a_map[pt1(w)] = apt;

								if(ok == 2)
								{
									new_pt.pt = pt1;
									new_pt.prev = pt.pt;
									to_check.push_back(new_pt);
								}
							}
						}
					}

					if(pt2.x >= 0 && pt2.y >= 0 && pt2.x < w - 1 && pt2.y < h - 1
						&& !IsBlocking(m[pt2(w)])
						&& !IsBlocking(m[c_dir2[i].x + pt.pt.x + pt.pt.y*w])
						&& !IsBlocking(m[pt.pt.x + (c_dir2[i].y + pt.pt.y)*w]))
					{
						bool ok = true;

						if(m[pt2(w)].type == DOORS)
						{
							Door* door = area.FindDoor(pt2);
							if(door && door->IsBlocking())
								ok = false;
						}

						if(ok && m[c_dir2[i].x + pt.pt.x + pt.pt.y*w].type == DOORS)
						{
							Door* door = area.FindDoor(Int2(c_dir2[i].x + pt.pt.x, pt.pt.y));
							if(door && door->IsBlocking())
								ok = false;
						}

						if(ok && m[pt.pt.x + (c_dir2[i].y + pt.pt.y)*w].type == DOORS)
						{
							Door* door = area.FindDoor(Int2(pt.pt.x, c_dir2[i].y + pt.pt.y));
							if(door && door->IsBlocking())
								ok = false;
						}

						if(ok)
						{
							apt.prev = pt.pt;
							apt.cost = prev_apt.cost + 15;
							apt.dist = (abs(pt2.x - target_tile.x) + abs(pt2.y - target_tile.y)) * 10;
							apt.sum = apt.dist + apt.cost;

							if(a_map[pt2(w)].IsLower(apt.sum))
							{
								apt.state = 1;
								a_map[pt2(w)] = apt;

								new_pt.pt = pt2;
								new_pt.prev = pt.pt;
								to_check.push_back(new_pt);
							}
						}
					}
				}
			}

			std::sort(to_check.begin(), to_check.end(), sorter);
		}

		// if target is not visited then path was not found
		if(a_map[target_tile(w)].state == 0)
			return false;

		// wype³nij elementami œcie¿kê
		path.push_back(target_tile);

		Int2 p;

		while((p = path.back()) != start_tile)
			path.push_back(a_map[p(w)].prev);
	}

	// fill path elements
	path.pop_back();

	return true;
}

//=================================================================================================
// 0 - path found
// 1 - start pos and target pos too far
// 2 - start location is blocked
// 3 - start tile and target tile is equal
// 4 - target tile is blocked
// 5 - path not found
int Pathfinding::FindLocalPath(LevelArea& area, vector<Int2>& path, const Int2& my_tile, const Int2& target_tile, const Unit* me, const Unit* other,
	const void* usable, bool is_end_point)
{
	assert(me);

	path.clear();
	if(marked == me)
		test_pf.clear();

	if(my_tile == target_tile)
		return 3;

	int dist = Int2::Distance(my_tile, target_tile);
	if(dist >= 32)
		return 1;

	// center
	const Int2 center_tile((my_tile + target_tile) / 2);

	int minx = max(area.mine.x * 8, center_tile.x - 15),
		miny = max(area.mine.y * 8, center_tile.y - 15),
		maxx = min(area.maxe.x * 8 - 1, center_tile.x + 16),
		maxy = min(area.maxe.y * 8 - 1, center_tile.y + 16);

	int w = maxx - minx,
		h = maxy - miny;

	uint size = (w + 1)*(h + 1);

	if(a_map.size() < size)
		a_map.resize(size);
	memset(&a_map[0], 0, sizeof(APoint)*size);
	if(local_pfmap.size() < size)
		local_pfmap.resize(size);

	const Unit* ignored_units[3] = { me, 0 };
	if(other)
		ignored_units[1] = other;
	Level::IgnoreObjects ignore = { 0 };
	ignore.ignored_units = (const Unit**)ignored_units;
	const void* ignored_objects[2] = { 0 };
	if(usable)
	{
		ignored_objects[0] = usable;
		ignore.ignored_objects = ignored_objects;
	}

	game_level->global_col.clear();
	game_level->GatherCollisionObjects(area, game_level->global_col, Box2d(float(minx) / 4 - 0.25f, float(miny) / 4 - 0.25f, float(maxx) / 4 + 0.25f, float(maxy) / 4 + 0.25f), &ignore);

	const float r = me->GetUnitRadius() - 0.25f / 2;

	if(marked == me)
	{
		for(int y = miny, y2 = 0; y < maxy; ++y, ++y2)
		{
			for(int x = minx, x2 = 0; x < maxx; ++x, ++x2)
			{
				if(!game_level->Collide(game_level->global_col, Box2d(0.25f*x, 0.25f*y, 0.25f*(x + 1), 0.25f*(y + 1)), r))
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
				local_pfmap[x2 + y2 * w] = game_level->Collide(game_level->global_col, Box2d(0.25f*x, 0.25f*y, 0.25f*(x + 1), 0.25f*(y + 1)), r);
		}
	}

	const Int2 target_rel(target_tile.x - minx, target_tile.y - miny),
		my_rel(my_tile.x - minx, my_tile.y - miny);

	// target tile is blocked
	if(!is_end_point && local_pfmap[target_rel(w)])
		return 4;

	// mark my tile as free
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

	bool have_empty_tile = false;
	for(int i = 0; i < 8; ++i)
	{
		Int2 pt = my_rel + neis[i];
		if(pt.x < 0 || pt.y < 0 || pt.x > 32 || pt.y > 32)
			continue;
		if(!local_pfmap[pt(w)])
		{
			have_empty_tile = true;
			break;
		}
	}

	if(!have_empty_tile)
		return 2;

	// add first tile to check
	static vector<Point> to_check;
	to_check.clear();
	{
		Point& pt = Add1(to_check);
		pt.pt = target_rel;
		pt.prev = Int2(0, 0);
	}

	APoint apt, prev_apt;
	apt.dist = apt.sum = Int2::Distance(my_rel, target_rel) * 10;
	apt.cost = 0;
	apt.state = 1;
	apt.prev = Int2(0, 0);
	a_map[target_rel(w)] = apt;

	AStarSort sorter(a_map, w);
	Point new_pt;

	const int MAX_STEPS = 100;
	int steps = 0;

	// begin search of path
	while(!to_check.empty())
	{
		Point pt = to_check.back();
		to_check.pop_back();
		prev_apt = a_map[pt.pt(w)];

		if(pt.pt == my_rel)
		{
			APoint& ept = a_map[pt.pt(w)];
			ept.state = 1;
			ept.prev = pt.prev;
			break;
		}

		for(int i = 0; i < 4; ++i)
		{
			const Int2& pt1 = c_dir[i] + pt.pt;
			const Int2& pt2 = c_dir2[i] + pt.pt;

			if(pt1.x >= 0 && pt1.y >= 0 && pt1.x < w - 1 && pt1.y < h - 1 && !local_pfmap[pt1(w)])
			{
				apt.prev = pt.pt;
				apt.cost = prev_apt.cost + 10;
				apt.dist = Int2::Distance(pt1, my_rel) * 10;
				apt.sum = apt.dist + apt.cost;

				if(a_map[pt1(w)].IsLower(apt.sum))
				{
					apt.state = 1;
					a_map[pt1(w)] = apt;

					new_pt.pt = pt1;
					new_pt.prev = pt.pt;
					to_check.push_back(new_pt);
				}
			}

			if(pt2.x >= 0 && pt2.y >= 0 && pt2.x < w - 1 && pt2.y < h - 1 && !local_pfmap[pt2(w)])
			{
				apt.prev = pt.pt;
				apt.cost = prev_apt.cost + 15;
				apt.dist = Int2::Distance(pt2, my_rel) * 10;
				apt.sum = apt.dist + apt.cost;

				if(a_map[pt2(w)].IsLower(apt.sum))
				{
					apt.state = 1;
					a_map[pt2(w)] = apt;

					new_pt.pt = pt2;
					new_pt.prev = pt.pt;
					to_check.push_back(new_pt);
				}
			}
		}

		++steps;
		if(steps > MAX_STEPS)
			break;

		std::sort(to_check.begin(), to_check.end(), sorter);
	}

	if(marked)
	{
		if(marked == me)
			test_pf_outside = (game_level->location->outside && me->area->area_type == LevelArea::Type::Outside);

		if(a_map[my_rel(w)].state == 0)
		{
			if(marked == me)
			{
				test_pf.push_back(pair<Vec2, int>(Vec2(0.25f*my_tile.x, 0.25f*my_tile.y), 1));
				test_pf.push_back(pair<Vec2, int>(Vec2(0.25f*target_tile.x, 0.25f*target_tile.y), 1));
			}
			return 5;
		}

		if(marked == me)
		{
			test_pf.push_back(pair<Vec2, int>(Vec2(0.25f*my_tile.x, 0.25f*my_tile.y), 1));
			test_pf.push_back(pair<Vec2, int>(Vec2(0.25f*target_tile.x, 0.25f*target_tile.y), 1));
		}

		// populate path vector (and debug drawing)
		Int2 p = my_rel;

		do
		{
			if(marked == me)
				test_pf.push_back(pair<Vec2, int>(Vec2(0.25f*(p.x + minx), 0.25f*(p.y + miny)), 1));
			path.push_back(Int2(p.x + minx, p.y + miny));
			p = a_map[p(w)].prev;
		}
		while(p != target_rel);
	}
	else
	{
		if(a_map[my_rel(w)].state == 0)
			return 5;

		// populate path vector (and debug drawing)
		Int2 p = my_rel;

		do
		{
			path.push_back(Int2(p.x + minx, p.y + miny));
			p = a_map[p(w)].prev;
		}
		while(p != target_rel);
	}

	path.push_back(target_tile);
	std::reverse(path.begin(), path.end());
	path.pop_back();

	return 0;
}

//=================================================================================================
void Pathfinding::Draw(BasicShader* shader)
{
	if(test_pf.empty() || !marked)
		return;

	shader->BeginBatch();

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
			float h = game_level->terrain->GetH(v[0].x, v[0].z) + 0.1f;
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

		shader->AddQuad(v, color);
	}

	shader->EndBatch();
}
