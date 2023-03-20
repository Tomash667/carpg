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
const Int2 cDir[4] = {
	Int2(0,1),
	Int2(1,0),
	Int2(0,-1),
	Int2(-1,0)
};

const Int2 cDir2[4] = {
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

	static vector<Point> toCheck;
	toCheck.clear();

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
		APoint apt, prevApt;
		apt.sum = apt.dist = 10 * (abs(targetTile.x - startTile.x) + abs(targetTile.y - startTile.y));
		apt.state = 1;
		apt.cost = 0;
		aMap[startTile(w)] = apt;

		Point pt, newPt;
		pt.pt = pt.prev = startTile;
		toCheck.push_back(pt);

		AStarSort sorter(aMap, w);

		// search for path
		while(!toCheck.empty())
		{
			pt = toCheck.back();
			toCheck.pop_back();

			apt = aMap[pt.pt(w)];
			prevApt = aMap[pt.prev(w)];

			if(pt.pt == targetTile)
			{
				APoint& ept = aMap[pt.pt(w)];
				ept.state = 1;
				ept.prev = pt.prev;
				break;
			}

			for(int i = 0; i < 4; ++i)
			{
				const Int2& pt1 = cDir[i] + pt.pt;
				const Int2& pt2 = cDir2[i] + pt.pt;

				if(pt1.x >= 0 && pt1.y >= 0 && pt1.x < w - 1 && pt1.y < w - 1 && aMap[pt1(w)].state == 0 && !m[pt1(w)].IsBlocking())
				{
					apt.prev = pt.pt;
					apt.cost = prevApt.cost + 10;
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

					newPt.pt = pt1;
					newPt.prev = pt.pt;
					toCheck.push_back(newPt);
				}

				if(pt2.x >= 0 && pt2.y >= 0 && pt2.x < w - 1 && pt2.y < w - 1 && aMap[pt2(w)].state == 0
					&& !m[pt2(w)].IsBlocking()
					&& !m[cDir2[i].x + pt.pt.x + pt.pt.y * w].IsBlocking()
					&& !m[pt.pt.x + (cDir2[i].y + pt.pt.y) * w].IsBlocking())
				{
					apt.prev = pt.pt;
					apt.cost = prevApt.cost + 15;
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

					newPt.pt = pt2;
					newPt.prev = pt.pt;
					toCheck.push_back(newPt);
				}
			}

			std::sort(toCheck.begin(), toCheck.end(), sorter);
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
		APoint apt, prevApt;
		apt.sum = apt.dist = 10 * (abs(targetTile.x - startTile.x) + abs(targetTile.y - startTile.y));
		apt.state = 1;
		apt.cost = 0;
		aMap[startTile(w)] = apt;

		Point pt, newPt;
		pt.pt = pt.prev = startTile;
		toCheck.push_back(pt);

		AStarSort sorter(aMap, w);

		// search for path
		while(!toCheck.empty())
		{
			pt = toCheck.back();
			toCheck.pop_back();
			prevApt = aMap[pt.pt(w)];

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
					const Int2& pt1 = cDir[i] + pt.pt;
					const Int2& pt2 = cDir2[i] + pt.pt;

					if(pt1.x >= 0 && pt1.y >= 0 && pt1.x < w - 1 && pt1.y < h - 1 && !IsBlocking(m[pt1(w)]))
					{
						apt.prev = pt.pt;
						apt.cost = prevApt.cost + 10;
						apt.dist = (abs(pt1.x - targetTile.x) + abs(pt1.y - targetTile.y)) * 10;
						apt.sum = apt.dist + apt.cost;

						if(aMap[pt1(w)].IsLower(apt.sum))
						{
							apt.state = 1;
							aMap[pt1(w)] = apt;

							newPt.pt = pt1;
							newPt.prev = pt.pt;
							toCheck.push_back(newPt);
						}
					}

					if(pt2.x >= 0 && pt2.y >= 0 && pt2.x < w - 1 && pt2.y < h - 1
						&& !IsBlocking(m[pt2(w)])
						&& !IsBlocking(m[cDir2[i].x + pt.pt.x + pt.pt.y * w])
						&& !IsBlocking(m[pt.pt.x + (cDir2[i].y + pt.pt.y) * w]))
					{
						apt.prev = pt.pt;
						apt.cost = prevApt.cost + 15;
						apt.dist = (abs(pt2.x - targetTile.x) + abs(pt2.y - targetTile.y)) * 10;
						apt.sum = apt.dist + apt.cost;

						if(aMap[pt2(w)].IsLower(apt.sum))
						{
							apt.state = 1;
							aMap[pt2(w)] = apt;

							newPt.pt = pt2;
							newPt.prev = pt.pt;
							toCheck.push_back(newPt);
						}
					}
				}
			}
			else
			{
				for(int i = 0; i < 4; ++i)
				{
					const Int2& pt1 = cDir[i] + pt.pt;
					const Int2& pt2 = cDir2[i] + pt.pt;

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
							apt.cost = prevApt.cost + 10;
							apt.dist = (abs(pt1.x - targetTile.x) + abs(pt1.y - targetTile.y)) * 10;
							apt.sum = apt.dist + apt.cost;

							if(aMap[pt1(w)].IsLower(apt.sum))
							{
								apt.state = 1;
								aMap[pt1(w)] = apt;

								if(ok == 2)
								{
									newPt.pt = pt1;
									newPt.prev = pt.pt;
									toCheck.push_back(newPt);
								}
							}
						}
					}

					if(pt2.x >= 0 && pt2.y >= 0 && pt2.x < w - 1 && pt2.y < h - 1
						&& !IsBlocking(m[pt2(w)])
						&& !IsBlocking(m[cDir2[i].x + pt.pt.x + pt.pt.y * w])
						&& !IsBlocking(m[pt.pt.x + (cDir2[i].y + pt.pt.y) * w]))
					{
						bool ok = true;

						if(m[pt2(w)].type == DOORS)
						{
							Door* door = locPart.FindDoor(pt2);
							if(door && door->IsBlocking())
								ok = false;
						}

						if(ok && m[cDir2[i].x + pt.pt.x + pt.pt.y * w].type == DOORS)
						{
							Door* door = locPart.FindDoor(Int2(cDir2[i].x + pt.pt.x, pt.pt.y));
							if(door && door->IsBlocking())
								ok = false;
						}

						if(ok && m[pt.pt.x + (cDir2[i].y + pt.pt.y) * w].type == DOORS)
						{
							Door* door = locPart.FindDoor(Int2(pt.pt.x, cDir2[i].y + pt.pt.y));
							if(door && door->IsBlocking())
								ok = false;
						}

						if(ok)
						{
							apt.prev = pt.pt;
							apt.cost = prevApt.cost + 15;
							apt.dist = (abs(pt2.x - targetTile.x) + abs(pt2.y - targetTile.y)) * 10;
							apt.sum = apt.dist + apt.cost;

							if(aMap[pt2(w)].IsLower(apt.sum))
							{
								apt.state = 1;
								aMap[pt2(w)] = apt;

								newPt.pt = pt2;
								newPt.prev = pt.pt;
								toCheck.push_back(newPt);
							}
						}
					}
				}
			}

			std::sort(toCheck.begin(), toCheck.end(), sorter);
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
int Pathfinding::FindLocalPath(LocationPart& locPart, vector<Int2>& path, const Int2& myTile, Int2 targetTile, const Unit* me, const Unit* other,
	const void* usable, Mode mode)
{
	assert(me);

	path.clear();
	if(marked == me)
		testPf.clear();

	if(myTile == targetTile)
		return 3;

	Int2 dir = targetTile - myTile;
	if(abs(dir.x) >= 31 || abs(dir.y) >= 31)
	{
		if(mode == MODE_SMART)
		{
			if(dir.x >= 31)
			{
				float ratio = 30.f / dir.x;
				dir.x = 30;
				dir.y = int(ratio * dir.y);
			}
			else if(dir.x <= -31)
			{
				float ratio = 30.f / -dir.x;
				dir.x = -30;
				dir.y = int(ratio * dir.y);
			}
			else if(dir.y >= 31)
			{
				float ratio = 30.f / dir.y;
				dir.y = 30;
				dir.x = int(ratio * dir.x);
			}
			else
			{
				float ratio = 30.f / -dir.y;
				dir.y = -30;
				dir.x = int(ratio * dir.x);
			}
			targetTile = myTile + dir;
		}
		else
			return 1;
	}

	// center
	const Int2 centerTile((myTile + targetTile) / 2);

	int minx = max(locPart.mine.x * 8, centerTile.x - 15),
		miny = max(locPart.mine.y * 8, centerTile.y - 15),
		maxx = min(locPart.maxe.x * 8 - 1, centerTile.x + 16),
		maxy = min(locPart.maxe.y * 8 - 1, centerTile.y + 16);

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

	const float r = me->GetRadius() - 0.25f / 2;

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

	// target tile is blocked?
	Int2 targetRel(targetTile.x - minx, targetTile.y - miny);
	if(localPfMap[targetRel(w)])
	{
		if(mode == MODE_NORMAL)
			return 4;
		else if(mode == MODE_SMART)
		{
			const Vec2 dirNormal = Vec2((float)dir.x, (float)dir.y).Normalized();
			float i = 1;
			while(true)
			{
				const Int2 newTargetTile = targetTile - Int2(dirNormal * i);
				if(newTargetTile == myTile)
					return 4;
				targetRel = Int2(newTargetTile.x - minx, newTargetTile.y - miny);
				if(!localPfMap[targetRel(w)])
				{
					targetTile = newTargetTile;
					break;
				}
				++i;
			}
		}
	}

	// mark my tile as free
	const Int2 myRel(myTile.x - minx, myTile.y - miny);
	localPfMap[myRel(w)] = false;
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

	bool haveEmptyTile = false;
	for(int i = 0; i < 8; ++i)
	{
		Int2 pt = myRel + neis[i];
		if(pt.x < 0 || pt.y < 0 || pt.x > 32 || pt.y > 32)
			continue;
		if(!localPfMap[pt(w)])
		{
			haveEmptyTile = true;
			break;
		}
	}

	if(!haveEmptyTile)
		return 2;

	// add first tile to check
	static vector<Point> toCheck;
	toCheck.clear();
	{
		Point& pt = Add1(toCheck);
		pt.pt = targetRel;
		pt.prev = Int2(0, 0);
	}

	APoint apt, prevApt;
	apt.dist = apt.sum = Int2::Distance(myRel, targetRel) * 10;
	apt.cost = 0;
	apt.state = 1;
	apt.prev = Int2(0, 0);
	aMap[targetRel(w)] = apt;

	AStarSort sorter(aMap, w);
	Point newPt;

	const int MAX_STEPS = 255;
	int steps = 0;

	// begin search of path
	while(!toCheck.empty())
	{
		Point pt = toCheck.back();
		toCheck.pop_back();
		prevApt = aMap[pt.pt(w)];

		if(pt.pt == myRel)
		{
			APoint& ept = aMap[pt.pt(w)];
			ept.state = 1;
			ept.prev = pt.prev;
			break;
		}

		for(int i = 0; i < 4; ++i)
		{
			const Int2& pt1 = cDir[i] + pt.pt;
			const Int2& pt2 = cDir2[i] + pt.pt;

			if(pt1.x >= 0 && pt1.y >= 0 && pt1.x < w && pt1.y < h && !localPfMap[pt1(w)])
			{
				apt.prev = pt.pt;
				apt.cost = prevApt.cost + 10;
				apt.dist = Int2::Distance(pt1, myRel) * 10;
				apt.sum = apt.dist + apt.cost;

				if(aMap[pt1(w)].IsLower(apt.sum))
				{
					apt.state = 1;
					aMap[pt1(w)] = apt;

					newPt.pt = pt1;
					newPt.prev = pt.pt;
					toCheck.push_back(newPt);
				}
			}

			if(pt2.x >= 0 && pt2.y >= 0 && pt2.x < w && pt2.y < h && !localPfMap[pt2(w)])
			{
				apt.prev = pt.pt;
				apt.cost = prevApt.cost + 15;
				apt.dist = Int2::Distance(pt2, myRel) * 10;
				apt.sum = apt.dist + apt.cost;

				if(aMap[pt2(w)].IsLower(apt.sum))
				{
					apt.state = 1;
					aMap[pt2(w)] = apt;

					newPt.pt = pt2;
					newPt.prev = pt.pt;
					toCheck.push_back(newPt);
				}
			}
		}

		++steps;
		if(steps > MAX_STEPS)
			break;

		std::sort(toCheck.begin(), toCheck.end(), sorter);
	}

	if(marked)
	{
		if(marked == me)
			testPfOutside = (gameLevel->location->outside && me->locPart->partType == LocationPart::Type::Outside);

		if(aMap[myRel(w)].state == 0)
		{
			if(marked == me)
			{
				testPf.push_back(pair<Vec2, int>(Vec2(0.25f * myTile.x, 0.25f * myTile.y), 1));
				testPf.push_back(pair<Vec2, int>(Vec2(0.25f * targetTile.x, 0.25f * targetTile.y), 1));
			}
			return 5;
		}

		if(marked == me)
		{
			testPf.push_back(pair<Vec2, int>(Vec2(0.25f * myTile.x, 0.25f * myTile.y), 1));
			testPf.push_back(pair<Vec2, int>(Vec2(0.25f * targetTile.x, 0.25f * targetTile.y), 1));
		}

		// populate path vector (and debug drawing)
		Int2 p = myRel;

		do
		{
			if(marked == me)
				testPf.push_back(pair<Vec2, int>(Vec2(0.25f * (p.x + minx), 0.25f * (p.y + miny)), 1));
			path.push_back(Int2(p.x + minx, p.y + miny));
			p = aMap[p(w)].prev;
		}
		while(p != targetRel);
	}
	else
	{
		if(aMap[myRel(w)].state == 0)
			return 5;

		// populate path vector (and debug drawing)
		Int2 p = myRel;

		do
		{
			path.push_back(Int2(p.x + minx, p.y + miny));
			p = aMap[p(w)].prev;
		}
		while(p != targetRel);
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
		array<Vec3, 4> v {
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
