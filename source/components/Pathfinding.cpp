#include "Pch.h"
#include "Pathfinding.h"

#include "InsideLocation.h"
#include "Level.h"
#include "OutsideLocation.h"

#include <BasicShader.h>
#include <Terrain.h>

//-----------------------------------------------------------------------------
const float SS = 0.25f;
Pathfinding* pathfinding;

//-----------------------------------------------------------------------------
struct Point
{
	Int2 pt, prev;
};

//-----------------------------------------------------------------------------
struct AStarSort
{
	AStarSort(vector<Pathfinding::APoint>& aMap, int size) : aMap(aMap), size(size)
	{
	}

	bool operator() (const Point& pt1, const Point& pt2) const
	{
		return aMap[pt1.pt.x + pt1.pt.y * size].sum > aMap[pt2.pt.x + pt2.pt.y * size].sum;
	}

	vector<Pathfinding::APoint>& aMap;
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
bool Pathfinding::FindPath(LocationPart& locPart, const Int2& startTile, const Int2& targetTile, vector<Int2>& path, bool canOpenDoors, bool wandering, vector<Int2>* blocked)
{
	if(startTile == targetTile || locPart.partType == LocationPart::Type::Building)
	{
		// already at same tile
		path.clear();
		path.push_back(targetTile);
		return true;
	}

	static vector<Point> to_check;
	to_check.clear();

	if(locPart.partType == LocationPart::Type::Outside)
	{
		// outside location
		OutsideLocation* outside = static_cast<OutsideLocation*>(gameLevel->location);
		const TerrainTile* m = outside->tiles;
		const int w = OutsideLocation::size;

		// is inside map?
		if(!outside->IsInside(startTile) || !outside->IsInside(targetTile))
			return false;

		// is in blocking tile?
		if(m[startTile(w)].IsBlocking() || m[targetTile(w)].IsBlocking())
			return false;

		// resize map
		const uint size = uint(w * w);
		if(size > aMap.size())
			aMap.resize(size);

		// zero out
		memset(&aMap[0], 0, sizeof(APoint) * size);
		path.clear();

		// apply blocked tiles
		if(blocked)
		{
			for(vector<Int2>::iterator it = blocked->begin(), end = blocked->end(); it != end; ++it)
				aMap[(*it)(w)].state = 1;
			if(aMap[startTile(w)].state == 1 || aMap[targetTile(w)].state == 1)
				return false;
		}

		// add first tile to check
		APoint apt, prev_apt;
		apt.sum = apt.dist = 10 * (abs(targetTile.x - startTile.x) + abs(targetTile.y - startTile.y));
		apt.state = 1;
		apt.cost = 0;
		aMap[startTile(w)] = apt;

		Point pt, new_pt;
		pt.pt = pt.prev = startTile;
		to_check.push_back(pt);

		AStarSort sorter(aMap, w);

		// search for path
		while(!to_check.empty())
		{
			pt = to_check.back();
			to_check.pop_back();

			apt = aMap[pt.pt(w)];
			prev_apt = aMap[pt.prev(w)];

			if(pt.pt == targetTile)
			{
				APoint& ept = aMap[pt.pt(w)];
				ept.state = 1;
				ept.prev = pt.prev;
				break;
			}

			for(int i = 0; i < 4; ++i)
			{
				const Int2& pt1 = c_dir[i] + pt.pt;
				const Int2& pt2 = c_dir2[i] + pt.pt;

				if(pt1.x >= 0 && pt1.y >= 0 && pt1.x < w - 1 && pt1.y < w - 1 && aMap[pt1(w)].state == 0 && !m[pt1(w)].IsBlocking())
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
					apt.dist = (abs(pt1.x - targetTile.x) + abs(pt1.y - targetTile.y)) * 10;
					apt.sum = apt.dist + apt.cost;
					apt.state = 1;
					aMap[pt1(w)] = apt;

					new_pt.pt = pt1;
					new_pt.prev = pt.pt;
					to_check.push_back(new_pt);
				}

				if(pt2.x >= 0 && pt2.y >= 0 && pt2.x < w - 1 && pt2.y < w - 1 && aMap[pt2(w)].state == 0
					&& !m[pt2(w)].IsBlocking()
					&& !m[c_dir2[i].x + pt.pt.x + pt.pt.y * w].IsBlocking()
					&& !m[pt.pt.x + (c_dir2[i].y + pt.pt.y) * w].IsBlocking())
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
					apt.dist = (abs(pt2.x - targetTile.x) + abs(pt2.y - targetTile.y)) * 10;
					apt.sum = apt.dist + apt.cost;
					apt.state = 1;
					aMap[pt2(w)] = apt;

					new_pt.pt = pt2;
					new_pt.prev = pt.pt;
					to_check.push_back(new_pt);
				}
			}

			std::sort(to_check.begin(), to_check.end(), sorter);
		}

		// if target is not visited then path was not found
		if(aMap[targetTile(w)].state == 0)
			return false;

		// fill path elements
		path.push_back(targetTile);

		Int2 p;

		while((p = path.back()) != startTile)
			path.push_back(aMap[p(w)].prev);
	}
	else
	{
		// inside location
		InsideLocation* inside = static_cast<InsideLocation*>(gameLevel->location);
		InsideLocationLevel& lvl = inside->GetLevelData();
		const Tile* m = lvl.map;
		const int w = lvl.w, h = lvl.h;

		// is inside level?
		if(!lvl.IsInside(startTile) || !lvl.IsInside(targetTile))
			return false;

		// is in blocking tile?
		if(IsBlocking(m[startTile(w)]) || IsBlocking(m[targetTile(w)]))
			return false;

		// resize map
		const uint size = uint(w * h);
		if(size > aMap.size())
			aMap.resize(size);

		// zero out
		memset(&aMap[0], 0, sizeof(APoint) * size);
		path.clear();

		// apply blocked tiles
		if(blocked)
		{
			for(vector<Int2>::iterator it = blocked->begin(), end = blocked->end(); it != end; ++it)
				aMap[(*it)(w)].state = 1;
			if(aMap[startTile(w)].state == 1 || aMap[targetTile(w)].state == 1)
				return false;
		}

		// add first tile to check
		APoint apt, prev_apt;
		apt.sum = apt.dist = 10 * (abs(targetTile.x - startTile.x) + abs(targetTile.y - startTile.y));
		apt.state = 1;
		apt.cost = 0;
		aMap[startTile(w)] = apt;

		Point pt, new_pt;
		pt.pt = pt.prev = startTile;
		to_check.push_back(pt);

		AStarSort sorter(aMap, w);

		// search for path
		while(!to_check.empty())
		{
			pt = to_check.back();
			to_check.pop_back();
			prev_apt = aMap[pt.pt(w)];

			if(pt.pt == targetTile)
			{
				APoint& ept = aMap[pt.pt(w)];
				ept.state = 1;
				ept.prev = pt.prev;
				break;
			}

			if(canOpenDoors)
			{
				for(int i = 0; i < 4; ++i)
				{
					const Int2& pt1 = c_dir[i] + pt.pt;
					const Int2& pt2 = c_dir2[i] + pt.pt;

					if(pt1.x >= 0 && pt1.y >= 0 && pt1.x < w - 1 && pt1.y < h - 1 && !IsBlocking(m[pt1(w)]))
					{
						apt.prev = pt.pt;
						apt.cost = prev_apt.cost + 10;
						apt.dist = (abs(pt1.x - targetTile.x) + abs(pt1.y - targetTile.y)) * 10;
						apt.sum = apt.dist + apt.cost;

						if(aMap[pt1(w)].IsLower(apt.sum))
						{
							apt.state = 1;
							aMap[pt1(w)] = apt;

							new_pt.pt = pt1;
							new_pt.prev = pt.pt;
							to_check.push_back(new_pt);
						}
					}

					if(pt2.x >= 0 && pt2.y >= 0 && pt2.x < w - 1 && pt2.y < h - 1
						&& !IsBlocking(m[pt2(w)])
						&& !IsBlocking(m[c_dir2[i].x + pt.pt.x + pt.pt.y * w])
						&& !IsBlocking(m[pt.pt.x + (c_dir2[i].y + pt.pt.y) * w]))
					{
						apt.prev = pt.pt;
						apt.cost = prev_apt.cost + 15;
						apt.dist = (abs(pt2.x - targetTile.x) + abs(pt2.y - targetTile.y)) * 10;
						apt.sum = apt.dist + apt.cost;

						if(aMap[pt2(w)].IsLower(apt.sum))
						{
							apt.state = 1;
							aMap[pt2(w)] = apt;

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
							Door* door = locPart.FindDoor(pt1);
							if(door && door->IsBlocking())
							{
								// decide if door can be entered from this tile
								if(IsBlocking(lvl.map[pt1.x - 1 + pt1.y * lvl.w].type))
								{
									// #   #
									// #---#
									// #   #
									int mov = 0;
									if(lvl.rooms[lvl.map[pt1.x + (pt1.y - 1) * lvl.w].room]->IsCorridor())
										++mov;
									if(lvl.rooms[lvl.map[pt1.x + (pt1.y + 1) * lvl.w].room]->IsCorridor())
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
									if(lvl.rooms[lvl.map[pt1.x - 1 + pt1.y * lvl.w].room]->IsCorridor())
										++mov;
									if(lvl.rooms[lvl.map[pt1.x + 1 + pt1.y * lvl.w].room]->IsCorridor())
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
							apt.dist = (abs(pt1.x - targetTile.x) + abs(pt1.y - targetTile.y)) * 10;
							apt.sum = apt.dist + apt.cost;

							if(aMap[pt1(w)].IsLower(apt.sum))
							{
								apt.state = 1;
								aMap[pt1(w)] = apt;

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
						&& !IsBlocking(m[c_dir2[i].x + pt.pt.x + pt.pt.y * w])
						&& !IsBlocking(m[pt.pt.x + (c_dir2[i].y + pt.pt.y) * w]))
					{
						bool ok = true;

						if(m[pt2(w)].type == DOORS)
						{
							Door* door = locPart.FindDoor(pt2);
							if(door && door->IsBlocking())
								ok = false;
						}

						if(ok && m[c_dir2[i].x + pt.pt.x + pt.pt.y * w].type == DOORS)
						{
							Door* door = locPart.FindDoor(Int2(c_dir2[i].x + pt.pt.x, pt.pt.y));
							if(door && door->IsBlocking())
								ok = false;
						}

						if(ok && m[pt.pt.x + (c_dir2[i].y + pt.pt.y) * w].type == DOORS)
						{
							Door* door = locPart.FindDoor(Int2(pt.pt.x, c_dir2[i].y + pt.pt.y));
							if(door && door->IsBlocking())
								ok = false;
						}

						if(ok)
						{
							apt.prev = pt.pt;
							apt.cost = prev_apt.cost + 15;
							apt.dist = (abs(pt2.x - targetTile.x) + abs(pt2.y - targetTile.y)) * 10;
							apt.sum = apt.dist + apt.cost;

							if(aMap[pt2(w)].IsLower(apt.sum))
							{
								apt.state = 1;
								aMap[pt2(w)] = apt;

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
		if(aMap[targetTile(w)].state == 0)
			return false;

		// wype³nij elementami œcie¿kê
		path.push_back(targetTile);

		Int2 p;

		while((p = path.back()) != startTile)
			path.push_back(aMap[p(w)].prev);
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
int Pathfinding::FindLocalPath(LocationPart& locPart, vector<Int2>& path, const Int2& my_tile, const Int2& targetTile, const Unit* me, const Unit* other,
	const void* usable, bool isEndPoint)
{
	assert(me);

	path.clear();
	if(marked == me)
		testPf.clear();

	if(my_tile == targetTile)
		return 3;

	int dist = Int2::Distance(my_tile, targetTile);
	if(dist >= 32)
		return 1;

	// center
	const Int2 center_tile((my_tile + targetTile) / 2);

	int minx = max(locPart.mine.x * 8, center_tile.x - 15),
		miny = max(locPart.mine.y * 8, center_tile.y - 15),
		maxx = min(locPart.maxe.x * 8 - 1, center_tile.x + 16),
		maxy = min(locPart.maxe.y * 8 - 1, center_tile.y + 16);

	int w = maxx - minx,
		h = maxy - miny;

	uint size = (w + 1) * (h + 1);

	if(aMap.size() < size)
		aMap.resize(size);
	memset(&aMap[0], 0, sizeof(APoint) * size);
	if(localPfMap.size() < size)
		localPfMap.resize(size);

	Level::IgnoreObjects ignore{};
	const Unit* ignoredUnits[3]{ me };
	if(other)
		ignoredUnits[1] = other;
	ignore.ignoredUnits = ignoredUnits;
	const void* ignoredObjects[2]{};
	if(usable)
	{
		ignoredObjects[0] = usable;
		ignore.ignoredObjects = ignoredObjects;
	}

	gameLevel->globalCol.clear();
	gameLevel->GatherCollisionObjects(locPart, gameLevel->globalCol, Box2d(float(minx) / 4 - 0.25f, float(miny) / 4 - 0.25f, float(maxx) / 4 + 0.25f, float(maxy) / 4 + 0.25f), &ignore);

	const float r = me->GetUnitRadius() - 0.25f / 2;

	if(marked == me)
	{
		for(int y = miny, y2 = 0; y < maxy; ++y, ++y2)
		{
			for(int x = minx, x2 = 0; x < maxx; ++x, ++x2)
			{
				if(!gameLevel->Collide(gameLevel->globalCol, Box2d(0.25f * x, 0.25f * y, 0.25f * (x + 1), 0.25f * (y + 1)), r))
				{
					testPf.push_back(pair<Vec2, int>(Vec2(0.25f * x, 0.25f * y), 0));
					localPfMap[x2 + y2 * w] = false;
				}
				else
					localPfMap[x2 + y2 * w] = true;
			}
		}
	}
	else
	{
		for(int y = miny, y2 = 0; y < maxy; ++y, ++y2)
		{
			for(int x = minx, x2 = 0; x < maxx; ++x, ++x2)
				localPfMap[x2 + y2 * w] = gameLevel->Collide(gameLevel->globalCol, Box2d(0.25f * x, 0.25f * y, 0.25f * (x + 1), 0.25f * (y + 1)), r);
		}
	}

	const Int2 target_rel(targetTile.x - minx, targetTile.y - miny),
		my_rel(my_tile.x - minx, my_tile.y - miny);

	// target tile is blocked
	if(!isEndPoint && localPfMap[target_rel(w)])
		return 4;

	// mark my tile as free
	localPfMap[my_rel(w)] = false;
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
		if(!localPfMap[pt(w)])
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
	aMap[target_rel(w)] = apt;

	AStarSort sorter(aMap, w);
	Point new_pt;

	const int MAX_STEPS = 100;
	int steps = 0;

	// begin search of path
	while(!to_check.empty())
	{
		Point pt = to_check.back();
		to_check.pop_back();
		prev_apt = aMap[pt.pt(w)];

		if(pt.pt == my_rel)
		{
			APoint& ept = aMap[pt.pt(w)];
			ept.state = 1;
			ept.prev = pt.prev;
			break;
		}

		for(int i = 0; i < 4; ++i)
		{
			const Int2& pt1 = c_dir[i] + pt.pt;
			const Int2& pt2 = c_dir2[i] + pt.pt;

			if(pt1.x >= 0 && pt1.y >= 0 && pt1.x < w - 1 && pt1.y < h - 1 && !localPfMap[pt1(w)])
			{
				apt.prev = pt.pt;
				apt.cost = prev_apt.cost + 10;
				apt.dist = Int2::Distance(pt1, my_rel) * 10;
				apt.sum = apt.dist + apt.cost;

				if(aMap[pt1(w)].IsLower(apt.sum))
				{
					apt.state = 1;
					aMap[pt1(w)] = apt;

					new_pt.pt = pt1;
					new_pt.prev = pt.pt;
					to_check.push_back(new_pt);
				}
			}

			if(pt2.x >= 0 && pt2.y >= 0 && pt2.x < w - 1 && pt2.y < h - 1 && !localPfMap[pt2(w)])
			{
				apt.prev = pt.pt;
				apt.cost = prev_apt.cost + 15;
				apt.dist = Int2::Distance(pt2, my_rel) * 10;
				apt.sum = apt.dist + apt.cost;

				if(aMap[pt2(w)].IsLower(apt.sum))
				{
					apt.state = 1;
					aMap[pt2(w)] = apt;

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
			testPfOutside = (gameLevel->location->outside && me->locPart->partType == LocationPart::Type::Outside);

		if(aMap[my_rel(w)].state == 0)
		{
			if(marked == me)
			{
				testPf.push_back(pair<Vec2, int>(Vec2(0.25f * my_tile.x, 0.25f * my_tile.y), 1));
				testPf.push_back(pair<Vec2, int>(Vec2(0.25f * targetTile.x, 0.25f * targetTile.y), 1));
			}
			return 5;
		}

		if(marked == me)
		{
			testPf.push_back(pair<Vec2, int>(Vec2(0.25f * my_tile.x, 0.25f * my_tile.y), 1));
			testPf.push_back(pair<Vec2, int>(Vec2(0.25f * targetTile.x, 0.25f * targetTile.y), 1));
		}

		// populate path vector (and debug drawing)
		Int2 p = my_rel;

		do
		{
			if(marked == me)
				testPf.push_back(pair<Vec2, int>(Vec2(0.25f * (p.x + minx), 0.25f * (p.y + miny)), 1));
			path.push_back(Int2(p.x + minx, p.y + miny));
			p = aMap[p(w)].prev;
		}
		while(p != target_rel);
	}
	else
	{
		if(aMap[my_rel(w)].state == 0)
			return 5;

		// populate path vector (and debug drawing)
		Int2 p = my_rel;

		do
		{
			path.push_back(Int2(p.x + minx, p.y + miny));
			p = aMap[p(w)].prev;
		}
		while(p != target_rel);
	}

	path.push_back(targetTile);
	std::reverse(path.begin(), path.end());
	path.pop_back();

	return 0;
}

//=================================================================================================
void Pathfinding::Draw(BasicShader* shader)
{
	if(testPf.empty() || !marked)
		return;

	for(vector<pair<Vec2, int>>::iterator it = testPf.begin(), end = testPf.end(); it != end; ++it)
	{
		Vec3 v[4] = {
			Vec3(it->first.x, 0.1f, it->first.y + SS),
			Vec3(it->first.x + SS, 0.1f, it->first.y + SS),
			Vec3(it->first.x, 0.1f, it->first.y),
			Vec3(it->first.x + SS, 0.1f, it->first.y)
		};

		if(testPfOutside)
		{
			float h = gameLevel->terrain->GetH(v[0].x, v[0].z) + 0.1f;
			for(int i = 0; i < 4; ++i)
				v[i].y = h;
		}

		const Color color[] =
		{
			Color(0.f, 1.f, 0.f, 0.5f),
			Color(1.f, 0.f, 0.f, 0.5f),
			Color(0.f, 0.f, 0.f, 0.5f)
		};

		shader->DrawQuad(v, color[it->second]);
	}

	shader->Draw();
}
