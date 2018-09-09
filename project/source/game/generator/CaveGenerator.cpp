#include "Pch.h"
#include "GameCore.h"
#include "CaveGenerator.h"
#include "Cave.h"
#include "Tile.h"
#include "Level.h"
#include "QuestManager.h"
#include "Quest_Mine.h"
#include "UnitGroup.h"
#include "Game.h"

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
int CaveGenerator::GetNumberOfSteps()
{
	int steps = LocationGenerator::GetNumberOfSteps();
	if(first)
		steps += 2; // txGeneratingObjects, txGeneratingUnits
	else if(!reenter)
		++steps; // txRegeneratingLevel
	return steps;
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
					if(m1[x + xx + (y + yy)*size])
						++count;
				}
			}

			m2[x + y * size] = (count >= 5);
		}
	}

	for(int i = 0; i < size; ++i)
	{
		m2[i] = m1[i];
		m2[i + (size - 1)*size] = m1[i + (size - 1)*size];
		m2[i*size] = m1[i*size];
		m2[i*size + size - 1] = m1[i*size + size - 1];
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
			T(x + (y - 1)*size);
		if(y < size - 1)
			T(x + (y + 1)*size);

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
	memset(m2, 1, sizeof(bool)*size2);
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
			T(x + (y - 1)*size);
		if(y < size - 1)
			T(x + (y + 1)*size);

#undef T
	}
	while(!v.empty());

	return c;
}

//=================================================================================================
int CaveGenerator::Finish(bool* m, bool* m2)
{
	memset(m2, 0, sizeof(bool)*size2);
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
			m1[j + (size - 1)*size] = true;
			m1[j*size + size - 1] = true;
			m1[j*size] = true;
		}
	}

	// select biggest area and fill it
	return Finish(m1, m2);
}

//=================================================================================================
void CaveGenerator::GenerateCave(Pole*& tiles, int size, Int2& stairs, int& stairs_dir, vector<Int2>& holes, Rect* ext)
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

	while(TryGenerate() < 200);

	tiles = new Pole[size2];
	memset(tiles, 0, sizeof(Pole)*size2);

	// set size
	if(ext)
		ext->Set(minx, miny, maxx + 1, maxy + 1);

	// copy tiles
	for(int i = 0; i < size2; ++i)
		tiles[i].type = (m2[i] ? SCIANA : PUSTE);

	CreateStairs(tiles, stairs, stairs_dir);
	CreateHoles(tiles, holes);

	Pole::SetupFlags(tiles, Int2(size, size));

	// rysuj
	if(Game::Get().devmode)
		DebugDraw();
}

//=================================================================================================
void CaveGenerator::CreateStairs(Pole* tiles, Int2& stairs, int& stairs_dir)
{
	do
	{
		Int2 pt, dir;

		switch(Rand() % 4)
		{
		case 0: // d�
			dir = Int2(0, -1);
			pt = Int2((Random(1, size - 2) + Random(1, size - 2)) / 2, size - 1);
			break;
		case 1: // lewa
			dir = Int2(1, 0);
			pt = Int2(0, (Random(1, size - 2) + Random(1, size - 2)) / 2);
			break;
		case 2: // g�ra
			dir = Int2(0, 1);
			pt = Int2((Random(1, size - 2) + Random(1, size - 2)) / 2, 0);
			break;
		case 3: // prawa
			dir = Int2(-1, 0);
			pt = Int2(size - 1, (Random(1, size - 2) + Random(1, size - 2)) / 2);
			break;
		}

		do
		{
			pt += dir;
			if(pt.x == -1 || pt.x == size || pt.y == -1 || pt.y == size)
				break;
			if(tiles[pt.x + pt.y*size].type == PUSTE)
			{
				pt -= dir;
				// sprawd� z ilu stron jest puste pole
				int count = 0, dir2;
				if(tiles[pt.x - 1 + pt.y*size].type == PUSTE)
				{
					++count;
					dir2 = 1;
				}
				if(tiles[pt.x + 1 + pt.y*size].type == PUSTE)
				{
					++count;
					dir2 = 3;
				}
				if(tiles[pt.x + (pt.y - 1)*size].type == PUSTE)
				{
					++count;
					dir2 = 0;
				}
				if(tiles[pt.x + (pt.y + 1)*size].type == PUSTE)
				{
					++count;
					dir2 = 2;
				}

				if(count == 1)
				{
					stairs = pt;
					stairs_dir = dir2;
					tiles[pt.x + pt.y*size].type = SCHODY_GORA;
					return;
				}
				else
					break;
			}
		} while(1);
	} while(1);
}

//=================================================================================================
void CaveGenerator::CreateHoles(Pole* tiles, vector<Int2>& holes)
{
	for(int count = 0, tries = 50; tries > 0 && count < 15; --tries)
	{
		Int2 pt(Random(1, size - 1), Random(1, size - 1));
		if(tiles[pt.x + pt.y*size].type == PUSTE)
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
				tiles[pt.x + pt.y*size].type = KRATKA_SUFIT;
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

	GenerateCave(lvl.map, 52, lvl.staircase_up, lvl.staircase_up_dir, cave->holes, &cave->ext);

	lvl.w = lvl.h = 52;
}

//=================================================================================================
void CaveGenerator::RegenerateFlags()
{
	InsideLocationLevel& lvl = GetLevelData();
	Pole::SetupFlags(lvl.map, Int2(lvl.w, lvl.h));
}

//=================================================================================================
void CaveGenerator::DebugDraw()
{
	for(int y = 0; y < size; ++y)
	{
		for(int x = 0; x < size; ++x)
			printf("%c", m1[x + y * size] ? '#' : ' ');
		printf("\n");
	}
	printf("\n");
}

//=================================================================================================
void CaveGenerator::GenerateObjects()
{
	Game& game = Game::Get();
	InsideLocationLevel& lvl = GetLevelData();

	Object* o = new Object;
	o->mesh = game.aStairsUp;
	o->pos = pt_to_pos(lvl.staircase_up);
	o->rot = Vec3(0, dir_to_rot(lvl.staircase_up_dir), 0);
	o->scale = 1;
	o->base = nullptr;
	game.local_ctx.objects->push_back(o);

	game.GenerateCaveObjects();
	if(L.location_index == QM.quest_mine->target_loc)
		QM.quest_mine->GenerateMine(this);
}

//=================================================================================================
void CaveGenerator::GenerateUnits()
{
	Game& game = Game::Get();

	// zbierz grupy
	static TmpUnitGroup e[3] = {
		{ UnitGroup::TryGet("wolfs") },
		{ UnitGroup::TryGet("spiders") },
		{ UnitGroup::TryGet("rats") }
	};

	static vector<Int2> tiles;
	Cave* cave = (Cave*)loc;
	InsideLocationLevel& lvl = cave->GetLevelData();
	int level = game.GetDungeonLevel();
	tiles.clear();
	tiles.push_back(lvl.staircase_up);

	// ustal wrog�w
	for(int i = 0; i < 3; ++i)
		e[i].Fill(level);

	for(int added = 0, tries = 50; added < 8 && tries>0; --tries)
	{
		Int2 pt = cave->GetRandomTile();
		if(lvl.map[pt.x + pt.y*lvl.w].type != PUSTE)
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
			// losuj grupe
			TmpUnitGroup& group = e[Rand() % 3];
			if(group.total == 0)
				continue;

			tiles.push_back(pt);
			++added;

			// postaw jednostki
			int levels = level * 2;
			while(levels > 0)
			{
				int k = Rand() % group.total, l = 0;
				UnitData* ud = nullptr;

				for(auto& entry : group.entries)
				{
					l += entry.count;
					if(k < l)
					{
						ud = entry.ud;
						break;
					}
				}

				assert(ud);

				if(!ud || ud->level.x > levels)
					break;

				int enemy_level = Random(ud->level.x, Min(ud->level.y, levels, level));
				if(!game.SpawnUnitNearLocation(game.local_ctx, Vec3(2.f*pt.x + 1.f, 0, 2.f*pt.y + 1.f), *ud, nullptr, enemy_level, 3.f))
					break;
				levels -= enemy_level;
			}
		}
	}
}

//=================================================================================================
void CaveGenerator::GenerateItems()
{
	GenerateMushrooms();
}

//=================================================================================================
bool CaveGenerator::HandleUpdate(int days)
{
	bool respawn_units;
	if(L.location_index == QM.quest_mine->target_loc)
		respawn_units = QM.quest_mine->GenerateMine(this);
	if(days > 0)
		GenerateMushrooms(min(days, 10));
	return respawn_units;
}

//=================================================================================================
void CaveGenerator::GenerateMushrooms(int days_since)
{
	Game& game = Game::Get();
	InsideLocationLevel& lvl = GetLevelData();
	Int2 pt;
	Vec2 pos;
	int dir;
	const Item* shroom = Item::Get("mushroom");

	for(int i = 0; i < days_since * 20; ++i)
	{
		pt = Int2::Random(Int2(1, 1), Int2(lvl.w - 2, lvl.h - 2));
		if(OR2_EQ(lvl.map[pt.x + pt.y*lvl.w].type, PUSTE, KRATKA_SUFIT) && lvl.IsTileNearWall(pt, dir))
		{
			pos.x = 2.f*pt.x;
			pos.y = 2.f*pt.y;
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
			game.SpawnGroundItemInsideRadius(shroom, pos, 0.5f);
		}
	}
}