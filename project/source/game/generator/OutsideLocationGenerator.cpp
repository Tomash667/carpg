#include "Pch.h"
#include "GameCore.h"
#include "OutsideLocationGenerator.h"
#include "OutsideLocation.h"
#include "OutsideObject.h"
#include "Item.h"
#include "Terrain.h"
#include "Game.h"

OutsideObject trees[] = {
	"tree", nullptr, Vec2(2,6),
	"tree2", nullptr, Vec2(2,6),
	"tree3", nullptr, Vec2(2,6)
};
const uint n_trees = countof(trees);

OutsideObject trees2[] = {
	"tree", nullptr, Vec2(4,8),
	"tree2", nullptr, Vec2(4,8),
	"tree3", nullptr, Vec2(4,8),
	"withered_tree", nullptr, Vec2(1,4)
};
const uint n_trees2 = countof(trees2);

OutsideObject misc[] = {
	"grass", nullptr, Vec2(1.f,1.5f),
	"grass", nullptr, Vec2(1.f,1.5f),
	"plant", nullptr, Vec2(1.0f,2.f),
	"plant", nullptr, Vec2(1.0f,2.f),
	"rock", nullptr, Vec2(1.f,1.f),
	"fern", nullptr, Vec2(1,2)
};
const uint n_misc = countof(misc);

void OutsideLocationGenerator::SpawnForestObjects(int road_dir)
{
	Game& game = Game::Get();
	Terrain* terrain = game.terrain;

	if(!trees[0].obj)
	{
		for(uint i = 0; i < n_trees; ++i)
			trees[i].obj = BaseObject::Get(trees[i].name);
		for(uint i = 0; i < n_trees2; ++i)
			trees2[i].obj = BaseObject::Get(trees2[i].name);
		for(uint i = 0; i < n_misc; ++i)
			misc[i].obj = BaseObject::Get(misc[i].name);
	}

	TerrainTile* tiles = ((OutsideLocation*)loc)->tiles;

	// obelisk
	if(Rand() % (road_dir == -1 ? 10 : 15) == 0)
	{
		Vec3 pos;
		if(road_dir == -1)
			pos = Vec3(127.f, 0, 127.f);
		else if(road_dir == 0)
			pos = Vec3(127.f, 0, Rand() % 2 == 0 ? 127.f - 32.f : 127.f + 32.f);
		else
			pos = Vec3(Rand() % 2 == 0 ? 127.f - 32.f : 127.f + 32.f, 0, 127.f);
		terrain->SetH(pos);
		pos.y -= 1.f;
		game.SpawnObjectEntity(game.local_ctx, BaseObject::Get("obelisk"), pos, 0.f);
	}
	else if(Rand() % 16 == 0)
	{
		// tree with rocks around it
		Vec3 pos(Random(48.f, 208.f), 0, Random(48.f, 208.f));
		pos.y = terrain->GetH(pos) - 1.f;
		game.SpawnObjectEntity(game.local_ctx, trees2[3].obj, pos, Random(MAX_ANGLE), 4.f);
		for(int i = 0; i < 12; ++i)
		{
			Vec3 pos2 = pos + Vec3(sin(PI * 2 * i / 12)*8.f, 0, cos(PI * 2 * i / 12)*8.f);
			pos2.y = terrain->GetH(pos2);
			game.SpawnObjectEntity(game.local_ctx, misc[4].obj, pos2, Random(MAX_ANGLE));
		}
	}

	// drzewa
	for(int i = 0; i < 1024; ++i)
	{
		Int2 pt(Random(1, OutsideLocation::size - 2), Random(1, OutsideLocation::size - 2));
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

	// inne
	for(int i = 0; i < 512; ++i)
	{
		Int2 pt(Random(1, OutsideLocation::size - 2), Random(1, OutsideLocation::size - 2));
		if(tiles[pt.x + pt.y*OutsideLocation::size].t != TT_SAND)
		{
			Vec3 pos(Random(2.f) + 2.f*pt.x, 0, Random(2.f) + 2.f*pt.y);
			pos.y = terrain->GetH(pos);
			OutsideObject& o = misc[Rand() % n_misc];
			game.SpawnObjectEntity(game.local_ctx, o.obj, pos, Random(MAX_ANGLE), o.scale.Random());
		}
	}
}

void OutsideLocationGenerator::SpawnForestItems(int count_mod)
{
	assert(InRange(count_mod, -2, 1));

	Game& game = Game::Get();

	// get count to spawn
	int herbs, green_herbs;
	switch(count_mod)
	{
	case -2:
		green_herbs = 0;
		herbs = Random(1, 3);
		break;
	case -1:
		green_herbs = 0;
		herbs = Random(2, 5);
		break;
	default:
	case 0:
		green_herbs = Random(0, 1);
		herbs = Random(5, 10);
		break;
	case 1:
		green_herbs = Random(1, 2);
		herbs = Random(10, 15);
		break;
	}

	// spawn items
	struct ItemToSpawn
	{
		const Item* item;
		int count;
	};
	ItemToSpawn items_to_spawn[] = {
		Item::Get("green_herb"), green_herbs,
		Item::Get("healing_herb"), herbs
	};
	TerrainTile* tiles = ((OutsideLocation*)loc)->tiles;
	Vec2 region_size(2.f, 2.f);
	for(const ItemToSpawn& to_spawn : items_to_spawn)
	{
		for(int i = 0; i < to_spawn.count; ++i)
		{
			for(int tries = 0; tries < 5; ++tries)
			{
				Int2 pt(Random(8, OutsideLocation::size - 9), Random(8, OutsideLocation::size - 9));
				TERRAIN_TILE type = tiles[pt.x + pt.y*OutsideLocation::size].t;
				if(type == TT_GRASS || type == TT_GRASS3)
				{
					game.SpawnGroundItemInsideRegion(to_spawn.item, Vec2(2.f*pt.x, 2.f*pt.y), region_size, false);
					break;
				}
			}
		}
	}
}
