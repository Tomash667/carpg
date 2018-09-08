#include "Pch.h"
#include "GameCore.h"
#include "MoonwellGenerator.h"
#include "OutsideLocation.h"
#include "OutsideObject.h"
#include "Terrain.h"
#include "Perlin.h"
#include "Level.h"
#include "Game.h"

void MoonwellGenerator::Generate()
{
	CreateMap();

	// randomize height
	terrain->SetHeightMap(outside->h);
	terrain->RandomizeHeight(0.f, 5.f);
	float* h = terrain->GetHeightMap();
	Perlin perlin(4, 4, 1);

	for(uint y = 0; y < s; ++y)
	{
		for(uint x = 0; x < s; ++x)
		{
			if(x < 15 || x > s - 15 || y < 15 || y > s - 15)
				h[x + y * (s + 1)] += Random(10.f, 15.f);
		}
	}

	terrain->RoundHeight();
	terrain->RoundHeight();

	for(uint y = 0; y < s; ++y)
	{
		for(uint x = 0; x < s; ++x)
			h[x + y * (s + 1)] += (perlin.Get(1.f / 256 * x, 1.f / 256 * y) + 1.f) * 5;
	}

	// set green hill in middle
	for(uint y = 40; y < s - 40; ++y)
	{
		for(uint x = 40; x < s - 40; ++x)
		{
			float d;
			if((d = Distance(float(x), float(y), 64.f, 64.f)) < 8.f)
			{
				outside->tiles[x + y * s].t = TT_GRASS;
				h[x + y * (s + 1)] += (1.f - d / 8.f) * 5;
			}
		}
	}

	terrain->RoundHeight();
	terrain->RoundHeight();
	terrain->RoundHeight();
	terrain->RemoveHeightMap();
}

void MoonwellGenerator::GenerateObjects()
{
	Game& game = Game::Get();

	Vec3 pos(128.f, 0, 128.f);
	terrain->SetH(pos);
	pos.y -= 0.2f;
	game.SpawnObjectEntity(game.local_ctx, BaseObject::Get("moonwell"), pos, 0.f);
	game.SpawnObjectEntity(game.local_ctx, BaseObject::Get("moonwell_phy"), pos, 0.f);

	TerrainTile* tiles = ((OutsideLocation*)L.location)->tiles;

	// drzewa
	for(int i = 0; i < 1024; ++i)
	{
		Int2 pt(Random(1, OutsideLocation::size - 2), Random(1, OutsideLocation::size - 2));
		if(Distance(float(pt.x), float(pt.y), 64.f, 64.f) > 5.f)
		{
			TERRAIN_TILE co = tiles[pt.x + pt.y*OutsideLocation::size].t;
			if(co == TT_GRASS)
			{
				Vec3 pos(Random(2.f) + 2.f*pt.x, 0, Random(2.f) + 2.f*pt.y);
				pos.y = terrain->GetH(pos);
				OutsideObject& o = trees[Rand() % n_trees];
				game.SpawnObjectEntity(game.local_ctx, o.obj, pos, Random(MAX_ANGLE), o.scale.Random());
			}
			else if(co == TT_GRASS3)
			{
				Vec3 pos(Random(2.f) + 2.f*pt.x, 0, Random(2.f) + 2.f*pt.y);
				pos.y = terrain->GetH(pos);
				int co;
				if(Rand() % 12 == 0)
					co = 3;
				else
					co = Rand() % 3;
				OutsideObject& o = trees2[co];
				game.SpawnObjectEntity(game.local_ctx, o.obj, pos, Random(MAX_ANGLE), o.scale.Random());
			}
		}
	}

	// inne
	for(int i = 0; i < 512; ++i)
	{
		Int2 pt(Random(1, OutsideLocation::size - 2), Random(1, OutsideLocation::size - 2));
		if(Distance(float(pt.x), float(pt.y), 64.f, 64.f) > 5.f)
		{
			if(tiles[pt.x + pt.y*OutsideLocation::size].t != TT_SAND)
			{
				Vec3 pos(Random(2.f) + 2.f*pt.x, 0, Random(2.f) + 2.f*pt.y);
				pos.y = terrain->GetH(pos);
				OutsideObject& o = misc[Rand() % n_misc];
				game.SpawnObjectEntity(game.local_ctx, o.obj, pos, Random(MAX_ANGLE), o.scale.Random());
			}
		}
	}
}

void MoonwellGenerator::GenerateUnits()
{
	Game::Get().SpawnMoonwellUnits(team_pos);
}

void MoonwellGenerator::GenerateItems()
{
	SpawnForestItems(1);
}
