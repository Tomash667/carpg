#include "Pch.h"
#include "GameCore.h"
#include "OutsideLocationGenerator.h"
#include "OutsideLocation.h"
#include "OutsideObject.h"
#include "Item.h"
#include "Terrain.h"
#include "Perlin.h"
#include "World.h"
#include "Level.h"
#include "QuestManager.h"
#include "Quest_Bandits.h"
#include "Team.h"
#include "Texture.h"
#include "Game.h"


const uint OutsideLocationGenerator::s = OutsideLocation::size;

OutsideObject OutsideLocationGenerator::trees[] = {
	"tree", nullptr, Vec2(2,6),
	"tree2", nullptr, Vec2(2,6),
	"tree3", nullptr, Vec2(2,6)
};
const uint OutsideLocationGenerator::n_trees = countof(trees);

OutsideObject OutsideLocationGenerator::trees2[] = {
	"tree", nullptr, Vec2(4,8),
	"tree2", nullptr, Vec2(4,8),
	"tree3", nullptr, Vec2(4,8),
	"withered_tree", nullptr, Vec2(1,4)
};
const uint OutsideLocationGenerator::n_trees2 = countof(trees2);

OutsideObject OutsideLocationGenerator::misc[] = {
	"grass", nullptr, Vec2(1.f,1.5f),
	"grass", nullptr, Vec2(1.f,1.5f),
	"plant", nullptr, Vec2(1.0f,2.f),
	"plant", nullptr, Vec2(1.0f,2.f),
	"rock", nullptr, Vec2(1.f,1.f),
	"fern", nullptr, Vec2(1,2)
};
const uint OutsideLocationGenerator::n_misc = countof(misc);


//=================================================================================================
void OutsideLocationGenerator::InitOnce()
{
	for(uint i = 0; i < n_trees; ++i)
		trees[i].obj = BaseObject::Get(trees[i].name);
	for(uint i = 0; i < n_trees2; ++i)
		trees2[i].obj = BaseObject::Get(trees2[i].name);
	for(uint i = 0; i < n_misc; ++i)
		misc[i].obj = BaseObject::Get(misc[i].name);
}

//=================================================================================================
void OutsideLocationGenerator::Init()
{
	outside = (OutsideLocation*)loc;
	terrain = Game::Get().terrain;
}

//=================================================================================================
int OutsideLocationGenerator::GetNumberOfSteps()
{
	int steps = LocationGenerator::GetNumberOfSteps();
	if(first)
		steps += 3; // txGeneratingObjects, txGeneratingUnits, txGeneratingItems
	else if(!reenter)
		steps += 2; // txGeneratingUnits, txGeneratingPhysics
	if(!reenter)
		++steps; // txRecreatingObjects
	return steps;
}

//=================================================================================================
void OutsideLocationGenerator::CreateMap()
{
	// create map
	if(!outside->tiles)
	{
		outside->tiles = new TerrainTile[s*s];
		outside->h = new float[(s + 1)*(s + 1)];
		memset(outside->tiles, 0, sizeof(TerrainTile)*s*s);
	}

	// set random grass texture
	Perlin perlin2(4, 4, 1);
	for(int i = 0, y = 0; y < s; ++y)
	{
		for(int x = 0; x < s; ++x, ++i)
		{
			int v = int((perlin2.Get(1.f / 256 * float(x), 1.f / 256 * float(y)) + 1.f) * 50);
			TERRAIN_TILE& t = outside->tiles[i].t;
			if(v < 42)
				t = TT_GRASS;
			else if(v < 58)
				t = TT_GRASS2;
			else
				t = TT_GRASS3;
		}
	}
}

//=================================================================================================
void OutsideLocationGenerator::OnEnter()
{
	Game& game = Game::Get();
	L.city_ctx = nullptr;
	if(!reenter)
		L.ApplyContext(outside, L.local_ctx);

	int days;
	bool need_reset = outside->CheckUpdate(days, W.GetWorldtime());
	int update = HandleUpdate();
	if(update != 1)
		need_reset = false;

	if(!reenter)
		game.ApplyTiles(outside->h, outside->tiles);

	game.SetOutsideParams();

	W.GetOutsideSpawnPoint(team_pos, team_dir);

	if(first)
	{
		// generate objects
		game.LoadingStep(game.txGeneratingObjects);
		GenerateObjects();

		// generate units
		game.LoadingStep(game.txGeneratingUnits);
		GenerateUnits();

		// generate items
		game.LoadingStep(game.txGeneratingItems);
		GenerateItems();
	}
	else if(!reenter)
	{
		if(days > 0)
			game.UpdateLocation(days, 100, false);

		if(need_reset)
		{
			// remove alive units
			for(vector<Unit*>::iterator it = L.local_ctx.units->begin(), end = L.local_ctx.units->end(); it != end; ++it)
			{
				if((*it)->IsAlive())
				{
					delete *it;
					*it = nullptr;
				}
			}
			RemoveNullElements(L.local_ctx.units);
		}

		// recreate colliders
		game.LoadingStep(game.txGeneratingPhysics);
		game.RespawnObjectColliders();

		// respawn units
		game.LoadingStep(game.txGeneratingUnits);
		if(update >= 0)
			L.RespawnUnits();
		if(need_reset)
			GenerateUnits();

		if(days > 10)
			GenerateItems();

		game.OnReenterLevel(L.local_ctx);
	}

	// create colliders
	if(!reenter)
	{
		game.LoadingStep(game.txRecreatingObjects);
		game.SpawnTerrainCollider();
		L.SpawnOutsideBariers();
	}

	// handle quest event
	if(outside->active_quest && outside->active_quest != (Quest_Dungeon*)ACTIVE_QUEST_HOLDER)
	{
		Quest_Event* event = outside->active_quest->GetEvent(L.location_index);
		if(event)
		{
			if(!event->done)
				game.HandleQuestEvent(event);
			L.event_handler = event->location_event_handler;
		}
	}

	// generate minimap
	game.LoadingStep(game.txGeneratingMinimap);
	CreateMinimap();

	SpawnTeam();

	// generate guards for bandits quest
	if(QM.quest_bandits->bandits_state == Quest_Bandits::State::GenerateGuards && L.location_index == QM.quest_bandits->target_loc)
	{
		QM.quest_bandits->bandits_state = Quest_Bandits::State::GeneratedGuards;
		UnitData* ud = UnitData::Get("guard_q_bandyci");
		int ile = Random(4, 5);
		Vec3 pos = team_pos + Vec3(sin(team_dir + PI) * 8, 0, cos(team_dir + PI) * 8);
		for(int i = 0; i < ile; ++i)
		{
			Unit* u = game.SpawnUnitNearLocation(L.local_ctx, pos, *ud, &Team.leader->pos, 6, 4.f);
			u->assist = true;
		}
	}
}

//=================================================================================================
void OutsideLocationGenerator::SpawnForestObjects(int road_dir)
{
	Game& game = Game::Get();
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
		game.SpawnObjectEntity(L.local_ctx, BaseObject::Get("obelisk"), pos, 0.f);
	}
	else if(Rand() % 16 == 0)
	{
		// tree with rocks around it
		Vec3 pos(Random(48.f, 208.f), 0, Random(48.f, 208.f));
		pos.y = terrain->GetH(pos) - 1.f;
		game.SpawnObjectEntity(L.local_ctx, trees2[3].obj, pos, Random(MAX_ANGLE), 4.f);
		for(int i = 0; i < 12; ++i)
		{
			Vec3 pos2 = pos + Vec3(sin(PI * 2 * i / 12)*8.f, 0, cos(PI * 2 * i / 12)*8.f);
			pos2.y = terrain->GetH(pos2);
			game.SpawnObjectEntity(L.local_ctx, misc[4].obj, pos2, Random(MAX_ANGLE));
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
			game.SpawnObjectEntity(L.local_ctx, o.obj, pos, Random(MAX_ANGLE), o.scale.Random());
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
			game.SpawnObjectEntity(L.local_ctx, o.obj, pos, Random(MAX_ANGLE), o.scale.Random());
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
			game.SpawnObjectEntity(L.local_ctx, o.obj, pos, Random(MAX_ANGLE), o.scale.Random());
		}
	}
}

//=================================================================================================
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

//=================================================================================================
int OutsideLocationGenerator::HandleUpdate()
{
	return 1;
}

//=================================================================================================
void OutsideLocationGenerator::SpawnTeam()
{
	Game::Get().AddPlayerTeam(team_pos, team_dir, reenter, true);
}

//=================================================================================================
void OutsideLocationGenerator::CreateMinimap()
{
	Game& game = Game::Get();
	TextureLock lock(game.tMinimap);

	for(int y = 0; y < OutsideLocation::size; ++y)
	{
		uint* pix = lock[y];
		for(int x = 0; x < OutsideLocation::size; ++x)
		{
			TERRAIN_TILE t = outside->tiles[x + (OutsideLocation::size - 1 - y)*OutsideLocation::size].t;
			Color col;
			if(t == TT_GRASS)
				col = Color(0, 128, 0);
			else if(t == TT_ROAD)
				col = Color(128, 128, 128);
			else if(t == TT_SAND)
				col = Color(128, 128, 64);
			else if(t == TT_GRASS2)
				col = Color(105, 128, 89);
			else if(t == TT_GRASS3)
				col = Color(127, 51, 0);
			else
				col = Color(255, 0, 0);
			if(x < 16 || x > 128 - 16 || y < 16 || y > 128 - 16)
			{
				col = ((col & 0xFF) / 2) |
					((((col & 0xFF00) >> 8) / 2) << 8) |
					((((col & 0xFF0000) >> 16) / 2) << 16) |
					0xFF000000;
			}

			*pix = col;
			++pix;
		}
	}

	game.minimap_size = OutsideLocation::size;
}

//=================================================================================================
void OutsideLocationGenerator::OnLoad()
{
	Game& game = Game::Get();
	OutsideLocation* outside = (OutsideLocation*)loc;

	game.SetOutsideParams();
	game.SetTerrainTextures();

	L.ApplyContext(outside, L.local_ctx);
	game.ApplyTiles(outside->h, outside->tiles);

	game.RespawnObjectColliders(false);
	game.SpawnTerrainCollider();

	if(outside->type == L_CITY)
	{
		L.city_ctx = (City*)loc;
		game.RespawnBuildingPhysics();
		game.SpawnCityPhysics();
	}
	else
	{
		L.city_ctx = nullptr;
		L.SpawnOutsideBariers();
	}

	game.InitQuadTree();
	game.CalculateQuadtree();

	CreateMinimap();
}
