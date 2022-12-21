#include "Pch.h"
#include "CaveGenerator.h"

#include "Cave.h"
#include "Game.h"
#include "GameResources.h"
#include "Level.h"
#include "QuestManager.h"
#include "Quest_Mine.h"
#include "Tile.h"
#include "UnitGroup.h"

//=================================================================================================
CaveGenerator::CaveGenerator() : m1(nullptr), m2(nullptr)
{
}

//=================================================================================================
CaveGenerator::~CaveGenerator()
{
	delete[] m1;
	delete[] m2;
}

//=================================================================================================
void CaveGenerator::FillMap(bool* m)
{
	for(int i = 0; i < size2; ++i)
		m[i] = (Rand() % 100 < fill);
}

//=================================================================================================
void CaveGenerator::Make(bool* m1, bool* m2)
{
	for(int y = 1; y < size - 1; ++y)
	{
		for(int x = 1; x < size - 1; ++x)
		{
			int count = 0;
			for(int yy = -1; yy <= 1; ++yy)
			{
				for(int xx = -1; xx <= 1; ++xx)
				{
					if(m1[x + xx + (y + yy) * size])
						++count;
				}
			}

			m2[x + y * size] = (count >= 5);
		}
	}

	for(int i = 0; i < size; ++i)
	{
		m2[i] = m1[i];
		m2[i + (size - 1) * size] = m1[i + (size - 1) * size];
		m2[i * size] = m1[i * size];
		m2[i * size + size - 1] = m1[i * size + size - 1];
	}
}

//=================================================================================================
int CaveGenerator::CalculateFill(bool* m, bool* m2, int start)
{
	int count = 0;
	v.push_back(start);
	m2[start] = true;

	do
	{
		int i = v.back();
		v.pop_back();
		++count;
		m2[i] = true;

		int x = i % size;
		int y = i / size;

#define T(x) if(!m[x] && !m2[x]) { m2[x] = true; v.push_back(x); }

		if(x > 0)
			T(x - 1 + y * size);
		if(x < size - 1)
			T(x + 1 + y * size);
		if(y < 0)
			T(x + (y - 1) * size);
		if(y < size - 1)
			T(x + (y + 1) * size);

#undef T
	}
	while(!v.empty());

	return count;
}

//=================================================================================================
int CaveGenerator::FillCave(bool* m, bool* m2, int start)
{
	v.push_back(start);
	m[start] = true;
	memset(m2, 1, sizeof(bool) * size2);
	int c = 1;

	do
	{
		int i = v.back();
		v.pop_back();

		int x = i % size;
		int y = i / size;

		if(x < minx)
			minx = x;
		if(y < miny)
			miny = y;
		if(x > maxx)
			maxx = x;
		if(y > maxy)
			maxy = y;

#define T(t) if(!m[t]) { v.push_back(t); m[t] = true; m2[t] = false; ++c; }

		if(x > 0)
			T(x - 1 + y * size);
		if(x < size - 1)
			T(x + 1 + y * size);
		if(y < 0)
			T(x + (y - 1) * size);
		if(y < size - 1)
			T(x + (y + 1) * size);

#undef T
	}
	while(!v.empty());

	return c;
}

//=================================================================================================
int CaveGenerator::Finish(bool* m, bool* m2)
{
	memset(m2, 0, sizeof(bool) * size2);
	int top = -1, topi = -1;

	// find bigest area
	for(int i = 0; i < size2; ++i)
	{
		if(!m[i] || m2[i])
			continue;

		int count = CalculateFill(m, m2, i);
		if(count > top)
		{
			top = count;
			topi = i;
		}
	}

	// fill area
	minx = size;
	miny = size;
	maxx = 0;
	maxy = 0;
	return FillCave(m, m2, topi);
}

//=================================================================================================
int CaveGenerator::TryGenerate()
{
	FillMap(m1);

	// celluar automata
	for(int i = 0; i < iter; ++i)
	{
		Make(m1, m2);
		bool* m = m1;
		m1 = m2;
		m2 = m;

		// borders
		for(int j = 0; j < size; ++j)
		{
			m1[j] = true;
			m1[j + (size - 1) * size] = true;
			m1[j * size + size - 1] = true;
			m1[j * size] = true;
		}
	}

	// select biggest area and fill it
	return Finish(m1, m2);
}

//=================================================================================================
void CaveGenerator::GenerateCave(Tile*& tiles, int size, Int2& stairs, GameDirection& stairsDir, vector<Int2>& holes, Rect* ext)
{
	assert(InRange(size, 10, 100));

	size2 = size * size;

	if(!m1 || this->size > size)
	{
		delete[] m1;
		delete[] m2;
		m1 = new bool[size2];
		m2 = new bool[size2];
	}

	this->size = size;

	while(TryGenerate() < 500);

	tiles = new Tile[size2];
	memset(tiles, 0, sizeof(Tile) * size2);

	// set size
	if(ext)
		ext->Set(minx, miny, maxx + 1, maxy + 1);

	// copy tiles
	for(int i = 0; i < size2; ++i)
		tiles[i].type = (m2[i] ? WALL : EMPTY);

	CreateStairs(tiles, stairs, stairsDir);
	CreateHoles(tiles, holes);

	Tile::SetupFlags(tiles, Int2(size, size), ENTRY_STAIRS_UP, ENTRY_STAIRS_DOWN);

	// rysuj
	if(game->devmode)
		DebugDraw();
}

//=================================================================================================
void CaveGenerator::CreateStairs(Tile* tiles, Int2& stairs, GameDirection& stairsDir)
{
	do
	{
		Int2 pt, dir;

		switch(Rand() % 4)
		{
		case GDIR_DOWN:
			dir = Int2(0, -1);
			pt = Int2((Random(1, size - 2) + Random(1, size - 2)) / 2, size - 1);
			break;
		case GDIR_LEFT:
			dir = Int2(1, 0);
			pt = Int2(0, (Random(1, size - 2) + Random(1, size - 2)) / 2);
			break;
		case GDIR_UP:
			dir = Int2(0, 1);
			pt = Int2((Random(1, size - 2) + Random(1, size - 2)) / 2, 0);
			break;
		case GDIR_RIGHT:
			dir = Int2(-1, 0);
			pt = Int2(size - 1, (Random(1, size - 2) + Random(1, size - 2)) / 2);
			break;
		}

		do
		{
			pt += dir;
			if(pt.x == -1 || pt.x == size || pt.y == -1 || pt.y == size)
				break;
			if(tiles[pt.x + pt.y * size].type == EMPTY)
			{
				pt -= dir;
				// sprawdü z ilu stron jest puste pole
				int count = 0;
				GameDirection stairsDirResult;
				if(tiles[pt.x - 1 + pt.y * size].type == EMPTY)
				{
					++count;
					stairsDirResult = GDIR_LEFT;
				}
				if(tiles[pt.x + 1 + pt.y * size].type == EMPTY)
				{
					++count;
					stairsDirResult = GDIR_RIGHT;
				}
				if(tiles[pt.x + (pt.y - 1) * size].type == EMPTY)
				{
					++count;
					stairsDirResult = GDIR_DOWN;
				}
				if(tiles[pt.x + (pt.y + 1) * size].type == EMPTY)
				{
					++count;
					stairsDirResult = GDIR_UP;
				}

				if(count == 1)
				{
					stairs = pt;
					stairsDir = stairsDirResult;
					tiles[pt.x + pt.y * size].type = ENTRY_PREV;
					return;
				}
				else
					break;
			}
		}
		while(1);
	}
	while(1);
}

//=================================================================================================
void CaveGenerator::CreateHoles(Tile* tiles, vector<Int2>& holes)
{
	for(int count = 0, tries = 50; tries > 0 && count < 15; --tries)
	{
		Int2 pt(Random(1, size - 1), Random(1, size - 1));
		if(tiles[pt.x + pt.y * size].type == EMPTY)
		{
			bool ok = true;
			for(vector<Int2>::iterator it = holes.begin(), end = holes.end(); it != end; ++it)
			{
				if(Int2::Distance(pt, *it) < 5)
				{
					ok = false;
					break;
				}
			}

			if(ok)
			{
				tiles[pt.x + pt.y * size].type = BARS_CEILING;
				holes.push_back(pt);
				++count;
			}
		}
	}
}

//=================================================================================================
void CaveGenerator::Generate()
{
	Cave* cave = (Cave*)loc;
	InsideLocationLevel& lvl = cave->GetLevelData();

	lvl.prevEntryType = ENTRY_STAIRS_UP;
	lvl.nextEntryPt = Int2(-1000, -1000);
	GenerateCave(lvl.map, 52, lvl.prevEntryPt, lvl.prevEntryDir, cave->holes, &cave->ext);

	lvl.w = lvl.h = 52;
}

//=================================================================================================
void CaveGenerator::RegenerateFlags()
{
	InsideLocationLevel& lvl = GetLevelData();
	Tile::SetupFlags(lvl.map, Int2(lvl.w, lvl.h), ENTRY_STAIRS_UP, ENTRY_STAIRS_DOWN);
}

//=================================================================================================
void CaveGenerator::DebugDraw()
{
	for(int y = size - 1; y >= 0; --y)
	{
		for(int x = 0; x < size; ++x)
			printf("%c", m2[x + y * size] ? '#' : ' ');
		printf("\n");
	}
	printf("\n");
}

//=================================================================================================
void CaveGenerator::GenerateObjects()
{
	InsideLocationLevel& lvl = GetLevelData();
	Cave* cave = (Cave*)inside;

	Object* o = new Object;
	o->mesh = gameRes->aStairsUp;
	o->pos = PtToPos(lvl.prevEntryPt);
	o->rot = Vec3(0, DirToRot(lvl.prevEntryDir), 0);
	o->scale = 1;
	o->base = nullptr;
	gameLevel->localPart->objects.push_back(o);

	// lights
	for(vector<Int2>::iterator it = cave->holes.begin(), end = cave->holes.end(); it != end; ++it)
	{
		GameLight& light = Add1(lvl.lights);
		light.startPos = Vec3(2.f * it->x + 1.f, 3.f, 2.f * it->y + 1.f);
		light.range = 5;
		light.startColor = Vec3(1.f, 1.0f, 1.0f);
	}

	// stalactites
	BaseObject* baseObj = BaseObject::Get("stalactite");
	sta.clear();
	for(int count = 0, tries = 200; count < 50 && tries>0; --tries)
	{
		Int2 pt = cave->GetRandomTile();
		if(lvl.map[pt.x + pt.y * lvl.w].type != EMPTY)
			continue;

		bool ok = true;
		for(vector<Int2>::iterator it = sta.begin(), end = sta.end(); it != end; ++it)
		{
			if(pt == *it)
			{
				ok = false;
				break;
			}
		}

		if(ok)
		{
			++count;

			Object* o = new Object;
			o->base = baseObj;
			o->mesh = baseObj->mesh;
			o->scale = Random(1.f, 2.f);
			o->rot = Vec3(0, Random(MAX_ANGLE), 0);
			o->pos = Vec3(2.f * pt.x + 1.f, 4.f, 2.f * pt.y + 1.f);
			gameLevel->localPart->objects.push_back(o);
			sta.push_back(pt);
		}
	}

	// plants
	baseObj = BaseObject::Get("plant2");
	for(int i = 0; i < 150; ++i)
	{
		Int2 pt = cave->GetRandomTile();

		if(lvl.map[pt.x + pt.y * lvl.w].type == EMPTY)
		{
			Object* o = new Object;
			o->base = baseObj;
			o->mesh = baseObj->mesh;
			o->scale = 1.f;
			o->rot = Vec3(0, Random(MAX_ANGLE), 0);
			o->pos = Vec3(2.f * pt.x + Random(0.1f, 1.9f), 0.f, 2.f * pt.y + Random(0.1f, 1.9f));
			gameLevel->localPart->objects.push_back(o);
		}
	}

	// mushrooms
	baseObj = BaseObject::Get("mushrooms");
	for(int i = 0; i < 100; ++i)
	{
		Int2 pt = cave->GetRandomTile();

		if(lvl.map[pt.x + pt.y * lvl.w].type == EMPTY)
		{
			Object* o = new Object;
			o->base = baseObj;
			o->mesh = baseObj->mesh;
			o->scale = 1.f;
			o->rot = Vec3(0, Random(MAX_ANGLE), 0);
			o->pos = Vec3(2.f * pt.x + Random(0.1f, 1.9f), 0.f, 2.f * pt.y + Random(0.1f, 1.9f));
			gameLevel->localPart->objects.push_back(o);
		}
	}

	// stalagmites
	baseObj = BaseObject::Get("stalagmite");
	BaseObject* baseObj2 = BaseObject::Get("stalagmite2");
	for(int i = 0; i < 80; ++i)
	{
		Int2 pt = cave->GetRandomTile();
		if(lvl.map[pt.x + pt.y * lvl.w].type == EMPTY)
			GenerateDungeonObject(lvl, pt, Rand() % 2 == 0 ? baseObj : baseObj2);
	}

	if(gameLevel->location == questMgr->questMine->targetLoc)
		questMgr->questMine->GenerateMine(this, true);
}

//=================================================================================================
void CaveGenerator::GenerateUnits()
{
	int level = gameLevel->GetDifficultyLevel();
	TmpUnitGroupList tmp;
	tmp.Fill(loc->group, level);
	static vector<Int2> tiles;
	Cave* cave = (Cave*)loc;
	InsideLocationLevel& lvl = cave->GetLevelData();
	tiles.clear();

	for(int added = 0, tries = 50; added < 8 && tries>0; --tries)
	{
		Int2 pt = cave->GetRandomTile();
		if(lvl.map[pt.x + pt.y * lvl.w].type != EMPTY)
			continue;

		if(Int2::Distance(pt, lvl.prevEntryPt) < (int)ALERT_SPAWN_RANGE)
			continue;
		bool ok = true;
		for(vector<Int2>::iterator it = tiles.begin(), end = tiles.end(); it != end; ++it)
		{
			if(Int2::Distance(pt, *it) < 10)
			{
				ok = false;
				break;
			}
		}

		if(ok)
		{
			tiles.push_back(pt);
			++added;
			for(TmpUnitGroup::Spawn& spawn : tmp.Roll(level, 2))
			{
				if(!gameLevel->SpawnUnitNearLocation(*gameLevel->localPart, Vec3(2.f * pt.x + 1.f, 0, 2.f * pt.y + 1.f), *spawn.first, nullptr, spawn.second, 3.f))
					break;
			}
		}
	}
}

//=================================================================================================
void CaveGenerator::GenerateItems()
{
	GenerateCaveItems();
}

//=================================================================================================
int CaveGenerator::HandleUpdate(int days)
{
	int updateFlags = 0;
	if(gameLevel->location == questMgr->questMine->targetLoc)
		updateFlags = questMgr->questMine->GenerateMine(this, false);
	if(days > 0)
		GenerateCaveItems(min(days, 10));
	return updateFlags;
}

//=================================================================================================
void CaveGenerator::GenerateCaveItems(int daysSince)
{
	InsideLocationLevel& lvl = GetLevelData();
	const Item* toSpawn[] = {
		Item::Get("mushroom"),
		Item::Get("magic_crystal"),
		Item::Get("heal_crystal")
	};

	for(int i = 0; i < daysSince * 20; ++i)
	{
		Int2 pt = Int2::Random(Int2(1, 1), Int2(lvl.w - 2, lvl.h - 2));
		if(!Any(lvl.map[pt.x + pt.y * lvl.w].type, EMPTY, BARS_CEILING))
			continue;

		int dir;
		if(lvl.IsTileNearWall(pt, dir))
		{
			Vec2 pos(2.f * pt.x, 2.f * pt.y);
			switch(dir)
			{
			case GDIR_LEFT:
				pos.x += 0.5f;
				pos.y += Random(-1.f, 1.f);
				break;
			case GDIR_RIGHT:
				pos.x += 1.5f;
				pos.y += Random(-1.f, 1.f);
				break;
			case GDIR_UP:
				pos.x += Random(-1.f, 1.f);
				pos.y += 1.5f;
				break;
			case GDIR_DOWN:
				pos.x += Random(-1.f, 1.f);
				pos.y += 0.5f;
				break;
			}
			gameLevel->SpawnGroundItemInsideRadius(toSpawn[Rand() % countof(toSpawn)], pos, 0.5f);
		}
		else if(Rand() % 3 == 0)
		{
			Vec2 pos(2.f * pt.x + Random(0.25f, 1.75f), 2.f * pt.y + Random(0.25f, 1.75f));
			gameLevel->SpawnGroundItemInsideRadius(toSpawn[Rand() % countof(toSpawn)], pos, 0.5f);
		}
	}
}
