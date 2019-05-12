#include "Pch.h"
#include "GameCore.h"
#include "LabyrinthGenerator.h"
#include "InsideLocation.h"
#include "Tile.h"
#include "Level.h"
#include "UnitGroup.h"
#include "Game.h"

//=================================================================================================
int LabyrinthGenerator::TryGenerate(const Int2& maze_size, const Int2& room_size, Int2& room_pos, Int2& doors)
{
	list<Int2> drillers;
	maze.resize(maze_size.x * maze_size.y, false);
	for(vector<bool>::iterator it = maze.begin(), end = maze.end(); it != end; ++it)
		*it = false;

	int mx = maze_size.x / 2 - room_size.x / 2,
		my = maze_size.y / 2 - room_size.y / 2;
	room_pos.x = mx;
	room_pos.y = my;

	for(int y = 0; y < room_size.y; ++y)
	{
		for(int x = 0; x < room_size.x; ++x)
		{
			maze[x + mx + (y + my)*maze_size.x] = true;
		}
	}

	Int2 start, start2;
	int dir;

	switch(Rand() % 4)
	{
	case 0:
		start.x = maze_size.x / 2;
		start.y = maze_size.y / 2 + room_size.y / 2;
		start2 = start;
		start.y--;
		dir = 1;
		break;
	case 1:
		start.x = maze_size.x / 2 - room_size.x / 2;
		start.y = maze_size.y / 2;
		start2 = start;
		start2.x--;
		dir = 2;
		break;
	case 2:
		start.x = maze_size.x / 2;
		start.y = maze_size.y / 2 - room_size.y / 2;
		start2 = start;
		start2.y--;
		dir = 0;
		break;
	case 3:
		start.x = maze_size.x / 2 + room_size.x / 2;
		start.y = maze_size.y / 2;
		start2 = start;
		start.x--;
		dir = 3;
		break;
	}
	drillers.push_back(start2);

	int pc = 0;
	while(!drillers.empty())
	{
		list<Int2>::iterator m, _m;
		m = drillers.begin();
		_m = drillers.end();
		while(m != _m)
		{
			bool remove_driller = false;
			switch(dir)
			{
			case 0:
				m->y -= 2;
				if(m->y < 0 || maze[m->y*maze_size.x + m->x])
				{
					remove_driller = true;
					break;
				}
				maze[(m->y + 1)*maze_size.x + m->x] = true;
				break;
			case 1:
				m->y += 2;
				if(m->y >= maze_size.y || maze[m->y*maze_size.x + m->x])
				{
					remove_driller = true;
					break;
				}
				maze[(m->y - 1)*maze_size.x + m->x] = true;
				break;
			case 2:
				m->x -= 2;
				if(m->x < 0 || maze[m->y*maze_size.x + m->x])
				{
					remove_driller = true;
					break;
				}
				maze[m->y*maze_size.x + m->x + 1] = true;
				break;
			case 3:
				m->x += 2;
				if(m->x >= maze_size.x || maze[m->y*maze_size.x + m->x])
				{
					remove_driller = true;
					break;
				}
				maze[m->y*maze_size.x + m->x - 1] = true;
				break;
			}
			if(remove_driller)
				m = drillers.erase(m);
			else
			{
				drillers.push_back(*m);
				// uncomment the line below to make the maze easier
				// if (Rand()%2)
				drillers.push_back(*m);

				maze[m->x + m->y*maze_size.x] = true;
				++m;
				++pc;
			}

			dir = Rand() % 4;
		}
	}

	for(int x = 0; x < room_size.x; ++x)
	{
		maze[mx + x + my * maze_size.x] = false;
		maze[mx + x + (my + room_size.y - 1)*maze_size.x] = false;
	}
	for(int y = 0; y < room_size.y; ++y)
	{
		maze[mx + (my + y)*maze_size.x] = false;
		maze[mx + room_size.x - 1 + (my + y)*maze_size.x] = false;
	}
	maze[start.x + start.y*maze_size.x] = true;
	maze[start2.x + start2.y*maze_size.x] = true;

	doors = Int2(start.x, start.y);

	return pc;
}

//=================================================================================================
void LabyrinthGenerator::GenerateLabyrinth(Tile*& tiles, const Int2& size, const Int2& room_size, Int2& stairs, GameDirection& stairs_dir, Int2& room_pos,
	int grating_chance)
{
	assert(room_size.x > 4 && room_size.y > 4 && size.x >= room_size.x * 2 + 4 && size.y >= room_size.y * 2 + 4);
	Int2 maze_size(size.x - 2, size.y - 2), doors;

	while(TryGenerate(maze_size, room_size, room_pos, doors) < 600);

	++room_pos.x;
	++room_pos.y;
	tiles = new Tile[size.x*size.y];
	memset(tiles, 0, sizeof(Tile)*size.x*size.y);

	// copy map
	for(int y = 0; y < maze_size.y; ++y)
	{
		for(int x = 0; x < maze_size.x; ++x)
			tiles[x + 1 + (y + 1)*size.x].type = (maze[x + y * maze_size.x] ? EMPTY : WALL);
	}

	tiles[doors.x + 1 + (doors.y + 1)*size.x].type = DOORS;

	// add blockades
	for(int x = 0; x < size.x; ++x)
	{
		tiles[x].type = BLOCKADE_WALL;
		tiles[x + (size.y - 1)*size.x].type = BLOCKADE_WALL;
	}

	for(int y = 1; y < size.y - 1; ++y)
	{
		tiles[y*size.x].type = BLOCKADE_WALL;
		tiles[size.x - 1 + y * size.x].type = BLOCKADE_WALL;
	}

	CreateStairs(tiles, size, stairs, stairs_dir);
	CreateGratings(tiles, size, room_size, room_pos, grating_chance);

	Tile::SetupFlags(tiles, size);

	if(Game::Get().devmode)
		Tile::DebugDraw(tiles, size);
}

//=================================================================================================
void LabyrinthGenerator::CreateStairs(Tile* tiles, const Int2& size, Int2& stairs, GameDirection& stairs_dir)
{
	int order[4] = { 0,1,2,3 };
	for(int i = 0; i < 5; ++i)
		std::swap(order[Rand() % 4], order[Rand() % 4]);
	bool ok = false;
	for(int i = 0; i < 4 && !ok; ++i)
	{
		switch(order[i])
		{
		case GDIR_DOWN:
			{
				int start = Random(1, size.x - 1);
				int p = start;
				do
				{
					if(tiles[p + size.x].type == EMPTY)
					{
						int count = 0;
						if(tiles[p - 1 + size.x].type != EMPTY)
							++count;
						if(tiles[p + 1 + size.x].type != EMPTY)
							++count;
						if(tiles[p].type != EMPTY)
							++count;
						if(tiles[p + size.x * 2].type != EMPTY)
							++count;

						if(count == 3)
						{
							stairs.x = p;
							stairs.y = 1;
							ok = true;
							break;
						}
					}
					++p;
					if(p == size.x)
						p = 1;
				} while(p != start);
			}
			break;
		case GDIR_LEFT:
			{
				int start = Random(1, size.y - 1);
				int p = start;
				do
				{
					if(tiles[1 + p * size.x].type == EMPTY)
					{
						int count = 0;
						if(tiles[p*size.x].type != EMPTY)
							++count;
						if(tiles[2 + p * size.x].type != EMPTY)
							++count;
						if(tiles[1 + (p - 1)*size.x].type != EMPTY)
							++count;
						if(tiles[1 + (p + 1)*size.x].type != EMPTY)
							++count;

						if(count == 3)
						{
							stairs.x = 1;
							stairs.y = p;
							ok = true;
							break;
						}
					}
					++p;
					if(p == size.x)
						p = 1;
				} while(p != start);
			}
			break;
		case GDIR_UP:
			{
				int start = Random(1, size.x - 1);
				int p = start;
				do
				{
					if(tiles[p + (size.y - 2)*size.x].type == EMPTY)
					{
						int count = 0;
						if(tiles[p - 1 + (size.y - 2)*size.x].type != EMPTY)
							++count;
						if(tiles[p + 1 + (size.y - 2)*size.x].type != EMPTY)
							++count;
						if(tiles[p + (size.y - 1)*size.x].type != EMPTY)
							++count;
						if(tiles[p + (size.y - 3)*size.x].type != EMPTY)
							++count;

						if(count == 3)
						{
							stairs.x = p;
							stairs.y = size.y - 2;
							ok = true;
							break;
						}
					}
					++p;
					if(p == size.x)
						p = 1;
				} while(p != start);
			}
			break;
		case GDIR_RIGHT:
			{
				int start = Random(1, size.y - 1);
				int p = start;
				do
				{
					if(tiles[size.x - 2 + p * size.x].type == EMPTY)
					{
						int count = 0;
						if(tiles[size.x - 3 + p * size.x].type != EMPTY)
							++count;
						if(tiles[size.x - 1 + p * size.x].type != EMPTY)
							++count;
						if(tiles[size.x - 2 + (p - 1)*size.x].type != EMPTY)
							++count;
						if(tiles[size.x - 2 + (p + 1)*size.x].type != EMPTY)
							++count;

						if(count == 3)
						{
							stairs.x = size.x - 2;
							stairs.y = p;
							ok = true;
							break;
						}
					}
					++p;
					if(p == size.x)
						p = 1;
				} while(p != start);
			}
			break;
		}
	}
	if(!ok)
		throw "Failed to generate labyrinth.";
	tiles[stairs.x + stairs.y*size.x].type = STAIRS_UP;

	// set stairs direction
	if(tiles[stairs.x + (stairs.y + 1)*size.x].type == EMPTY)
		stairs_dir = GDIR_UP;
	else if(tiles[stairs.x - 1 + stairs.y*size.x].type == EMPTY)
		stairs_dir = GDIR_LEFT;
	else if(tiles[stairs.x + (stairs.y - 1)*size.x].type == EMPTY)
		stairs_dir = GDIR_DOWN;
	else if(tiles[stairs.x + 1 + stairs.y*size.x].type == EMPTY)
		stairs_dir = GDIR_RIGHT;
	else
	{
		assert(0);
	}
}

//=================================================================================================
void LabyrinthGenerator::CreateGratings(Tile* tiles, const Int2& size, const Int2& room_size, const Int2& room_pos, int grating_chance)
{
	if(grating_chance <= 0)
		return;

	for(int y = 1; y < size.y - 1; ++y)
	{
		for(int x = 1; x < size.x - 1; ++x)
		{
			Tile& p = tiles[x + y * size.x];
			if(p.type == EMPTY && !Rect::IsInside(room_pos, room_size, Int2(x,y)) && Rand() % 100 < grating_chance)
			{
				int j = Rand() % 3;
				if(j == 0)
					p.type = BARS_FLOOR;
				else if(j == 1)
					p.type = BARS_CEILING;
				else
					p.type = BARS;
			}
		}
	}
}

//=================================================================================================
void LabyrinthGenerator::Generate()
{
	InsideLocation* inside = (InsideLocation*)loc;
	BaseLocation& base = g_base_locations[inside->target];
	InsideLocationLevel& lvl = inside->GetLevelData();

	assert(!lvl.map);

	Int2 room_pos;
	GenerateLabyrinth(lvl.map, Int2(base.size, base.size), base.room_size, lvl.staircase_up, lvl.staircase_up_dir, room_pos, base.bars_chance);

	lvl.w = lvl.h = base.size;
	Room* room = Room::Get();
	room->target = RoomTarget::None;
	room->pos = room_pos;
	room->size = base.room_size;
	room->connected.clear();
	lvl.rooms.push_back(room);
}

//=================================================================================================
void LabyrinthGenerator::GenerateObjects()
{
	InsideLocationLevel& lvl = GetLevelData();

	GenerateDungeonObjects();
	GenerateTraps();

	BaseObject* torch = BaseObject::Get("torch");

	// torch near wall
	Int2 pt = lvl.GetUpStairsFrontTile();
	Vec3 pos;
	if(IsBlocking(lvl.map[pt.x - 1 + pt.y*lvl.w].type))
		pos = Vec3(2.f*pt.x + torch->size.x + 0.1f, 0.f, 2.f*pt.y + 1.f);
	else if(IsBlocking(lvl.map[pt.x + 1 + pt.y*lvl.w].type))
		pos = Vec3(2.f*(pt.x + 1) - torch->size.x - 0.1f, 0.f, 2.f*pt.y + 1.f);
	else if(IsBlocking(lvl.map[pt.x + (pt.y - 1)*lvl.w].type))
		pos = Vec3(2.f*pt.x + 1.f, 0.f, 2.f*pt.y + torch->size.y + 0.1f);
	else if(IsBlocking(lvl.map[pt.x + (pt.y + 1)*lvl.w].type))
		pos = Vec3(2.f*pt.x + 1.f, 0.f, 2.f*(pt.y + 1) + torch->size.y - 0.1f);
	L.SpawnObjectEntity(L.local_ctx, torch, pos, Random(MAX_ANGLE));

	// torch inside treasure
	L.SpawnObjectEntity(L.local_ctx, torch, lvl.rooms[0]->Center(), Random(MAX_ANGLE));
}

//=================================================================================================
void LabyrinthGenerator::GenerateUnits()
{
	// decide what to spawn
	int count, tries;
	if(L.location->group->id == "unk")
	{
		count = 30;
		tries = 150;
	}
	else
	{
		count = 20;
		tries = 100;
	}
	int level = L.GetDifficultyLevel();
	Pooled<TmpUnitGroup> t;
	t->Fill(L.location->group, level);

	// spawn units
	InsideLocationLevel& lvl = ((InsideLocation*)L.location)->GetLevelData();
	for(int added = 0; added < count && tries; --tries)
	{
		Int2 pt(Random(1, lvl.w - 2), Random(1, lvl.h - 2));
		if(IsBlocking2(lvl.map[pt(lvl.w)]))
			continue;
		if(Int2::Distance(pt, lvl.staircase_up) < 5)
			continue;

		TmpUnitGroup::Spawn spawn = t->Get();
		if(L.SpawnUnitNearLocation(L.local_ctx, Vec3(2.f*pt.x + 1.f, 0, 2.f*pt.y + 1.f), *spawn.first, nullptr, spawn.second))
			++added;
	}

	// enemies inside treasure room
	if(L.location->group->id == "unk")
	{
		for(int i = 0; i < 3; ++i)
		{
			TmpUnitGroup::Spawn spawn = t->Get();
			L.SpawnUnitInsideRoom(*lvl.rooms[0], *spawn.first, spawn.second);
		}
	}
}
