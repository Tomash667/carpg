#include "Pch.h"
#include "OutsideLocationGenerator.h"

#include "Game.h"
#include "Item.h"
#include "Level.h"
#include "LevelPart.h"
#include "OutsideLocation.h"
#include "OutsideObject.h"
#include "QuestManager.h"
#include "Quest_Bandits.h"
#include "Team.h"
#include "World.h"

#include <Perlin.h>
#include <Scene.h>
#include <Terrain.h>
#include <Texture.h>

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
	outside = static_cast<OutsideLocation*>(loc);
	terrain = game_level->terrain;
}

//=================================================================================================
int OutsideLocationGenerator::GetNumberOfSteps()
{
	int steps = LocationGenerator::GetNumberOfSteps();
	if(first)
		steps += 3; // txGeneratingObjects, txGeneratingUnits, txGeneratingItems
	else
		steps += 2; // txGeneratingUnits, txGeneratingPhysics
	++steps; // txRecreatingObjects
	return steps;
}

//=================================================================================================
void OutsideLocationGenerator::SetOutsideParams()
{
	Scene* scene = outside->lvlPart->scene;
	scene->clear_color = Color::White;
	scene->fog_range = Vec2(40, 80);
	scene->fog_color = Color(0.9f, 0.85f, 0.8f);
	scene->ambient_color = Color(0.5f, 0.5f, 0.5f);
	scene->light_color = Color::White;
	scene->light_dir = Vec3(sin(game_level->light_angle), 2.f, cos(game_level->light_angle)).Normalize();
	scene->use_light_dir = true;
	outside->lvlPart->draw_range = 80.f;
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
}

//=================================================================================================
void OutsideLocationGenerator::RandomizeTerrainTexture()
{
	// set random grass texture
	Perlin perlin2(4, 4);
	for(uint i = 0, y = 0; y < s; ++y)
	{
		for(uint x = 0; x < s; ++x, ++i)
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
void OutsideLocationGenerator::RandomizeHeight(int octaves, float frequency, float hmin, float hmax)
{
	static Perlin perlin;
	const int w = OutsideLocation::size; // allows non-square terrain (but there isn't any yet)
	const int h = OutsideLocation::size;
	float* height = outside->h;
	perlin.Change(octaves, frequency);
	const float hdif = hmax - hmin;

	for(int y = 0; y <= h; ++y)
	{
		for(int x = 0; x <= w; ++x)
		{
			const float value = perlin.GetNormalized(1.f / (w + 1)*x, 1.f / (h + 1)*y);
			height[x + y * (w + 1)] = hmin + hdif * value;
		}
	}

	for(int y = 0; y < h; ++y)
	{
		for(int x = 0; x < w; ++x)
		{
			if(x < 15 || x > w - 15 || y < 15 || y > h - 15)
				height[x + y * (w + 1)] += Random(5.f, 10.f);
		}
	}
}

//=================================================================================================
void OutsideLocationGenerator::OnEnter()
{
	game_level->Apply();
	ApplyTiles();

	int days;
	bool need_reset = outside->CheckUpdate(days, world->GetWorldtime());
	int update_flags = HandleUpdate(days);
	if(IsSet(update_flags, PREVENT_RESET))
		need_reset = false;

	SetOutsideParams();

	world->GetOutsideSpawnPoint(team_pos, team_dir);

	if(first)
	{
		// generate objects
		game->LoadingStep(game->txGeneratingObjects);
		GenerateObjects();

		// generate units
		game->LoadingStep(game->txGeneratingUnits);
		GenerateUnits();

		// generate items
		game->LoadingStep(game->txGeneratingItems);
		GenerateItems();
	}
	else
	{
		if(days > 0)
			game_level->UpdateLocation(days, 100, need_reset);

		game_level->RecreateTmpObjectPhysics();

		// recreate colliders
		game->LoadingStep(game->txGeneratingPhysics);
		if(!IsSet(update_flags, PREVENT_RECREATE_OBJECTS))
			game_level->RecreateObjects();

		// respawn units
		game->LoadingStep(game->txGeneratingUnits);
		if(!IsSet(update_flags, PREVENT_RESPAWN_UNITS))
			RespawnUnits();
		if(need_reset)
			GenerateUnits();

		if(days > 10)
			GenerateItems();

		game_level->OnRevisitLevel();
	}

	// create colliders
	game->LoadingStep(game->txRecreatingObjects);
	game_level->SpawnTerrainCollider();
	SpawnOutsideBariers();

	// handle quest event
	if(outside->active_quest && outside->active_quest != ACTIVE_QUEST_HOLDER)
	{
		Quest_Dungeon* quest = dynamic_cast<Quest_Dungeon*>(outside->active_quest);
		Quest_Event* event = quest ? quest->GetEvent(game_level->location) : nullptr;
		if(event)
		{
			if(!event->done)
				quest_mgr->HandleQuestEvent(event);
			game_level->event_handler = event->location_event_handler;
		}
	}

	// generate minimap
	game->LoadingStep(game->txGeneratingMinimap);
	CreateMinimap();

	SpawnTeam();

	// generate guards for bandits quest
	if(quest_mgr->quest_bandits->bandits_state == Quest_Bandits::State::GenerateGuards && game_level->location == quest_mgr->quest_bandits->targetLoc)
	{
		quest_mgr->quest_bandits->bandits_state = Quest_Bandits::State::GeneratedGuards;
		UnitData* ud = UnitData::Get("guard_q_bandyci");
		int count = Random(4, 5);
		Vec3 pos = team_pos + Vec3(sin(team_dir + PI) * 8, 0, cos(team_dir + PI) * 8);
		for(int i = 0; i < count; ++i)
		{
			Unit* u = game_level->SpawnUnitNearLocation(*outside, pos, *ud, &team->leader->pos, 6, 4.f);
			u->assist = true;
		}
	}

	game_level->RemoveTmpObjectPhysics();
}

//=================================================================================================
void OutsideLocationGenerator::SpawnForestObjects(int road_dir)
{
	LocationPart& locPart = *outside;
	TerrainTile* tiles = outside->tiles;

	if(Rand() % (road_dir == -1 ? 10 : 15) == 0)
	{
		// obelisk
		Vec3 pos;
		if(road_dir == -1)
			pos = Vec3(127.f, 0, 127.f);
		else if(road_dir == 0)
			pos = Vec3(127.f, 0, Rand() % 2 == 0 ? 127.f - 32.f : 127.f + 32.f);
		else
			pos = Vec3(Rand() % 2 == 0 ? 127.f - 32.f : 127.f + 32.f, 0, 127.f);
		terrain->SetY(pos);
		pos.y -= 1.f;
		game_level->SpawnObjectEntity(locPart, BaseObject::Get("obelisk"), pos, 0.f);
	}
	else if(Rand() % 16 == 0)
	{
		// tree with rocks around it
		Vec3 pos(Random(48.f, 208.f), 0, Random(48.f, 208.f));
		pos.y = terrain->GetH(pos) - 1.f;
		game_level->SpawnObjectEntity(locPart, trees2[3].obj, pos, Random(MAX_ANGLE), 4.f);
		for(int i = 0; i < 12; ++i)
		{
			Vec3 pos2 = pos + Vec3(sin(PI * 2 * i / 12)*8.f, 0, cos(PI * 2 * i / 12)*8.f);
			pos2.y = terrain->GetH(pos2);
			game_level->SpawnObjectEntity(locPart, misc[4].obj, pos2, Random(MAX_ANGLE));
		}
	}

	// trees
	for(int i = 0; i < 1024; ++i)
	{
		Int2 pt(Random(1, OutsideLocation::size - 2), Random(1, OutsideLocation::size - 2));
		TERRAIN_TILE tile = tiles[pt.x + pt.y*OutsideLocation::size].t;
		if(tile == TT_GRASS)
		{
			Vec3 pos(Random(2.f) + 2.f*pt.x, 0, Random(2.f) + 2.f*pt.y);
			pos.y = terrain->GetH(pos);
			OutsideObject& o = trees[Rand() % n_trees];
			game_level->SpawnObjectEntity(locPart, o.obj, pos, Random(MAX_ANGLE), o.scale.Random());
		}
		else if(tile == TT_GRASS3)
		{
			Vec3 pos(Random(2.f) + 2.f*pt.x, 0, Random(2.f) + 2.f*pt.y);
			pos.y = terrain->GetH(pos);
			int type;
			if(Rand() % 12 == 0)
				type = 3;
			else
				type = Rand() % 3;
			OutsideObject& o = trees2[type];
			game_level->SpawnObjectEntity(locPart, o.obj, pos, Random(MAX_ANGLE), o.scale.Random());
		}
	}

	// other objects
	for(int i = 0; i < 512; ++i)
	{
		Int2 pt(Random(1, OutsideLocation::size - 2), Random(1, OutsideLocation::size - 2));
		if(tiles[pt.x + pt.y*OutsideLocation::size].t != TT_SAND)
		{
			Vec3 pos(Random(2.f) + 2.f*pt.x, 0, Random(2.f) + 2.f*pt.y);
			pos.y = terrain->GetH(pos);
			OutsideObject& o = misc[Rand() % n_misc];
			game_level->SpawnObjectEntity(locPart, o.obj, pos, Random(MAX_ANGLE), o.scale.Random());
		}
	}
}

//=================================================================================================
void OutsideLocationGenerator::SpawnForestItems(int countMod)
{
	assert(InRange(countMod, -2, 1));

	// get count to spawn
	int healingHerbs, manaHerbs, greenHerbs;
	switch(countMod)
	{
	case -2:
		greenHerbs = 0;
		healingHerbs = Random(1, 2);
		manaHerbs = Random(1, 2);
		break;
	case -1:
		greenHerbs = 0;
		healingHerbs = Random(2, 4);
		manaHerbs = Random(2, 4);
		break;
	default:
	case 0:
		greenHerbs = Random(0, 1);
		healingHerbs = Random(4, 8);
		manaHerbs = Random(4, 8);
		break;
	case 1:
		greenHerbs = Random(1, 2);
		healingHerbs = Random(8, 12);
		manaHerbs = Random(8, 12);
		break;
	}

	// spawn items
	struct ItemToSpawn
	{
		const Item* item;
		int count;
	};
	const ItemToSpawn itemsToSpawn[] = {
		Item::Get("green_herb"), greenHerbs,
		Item::Get("healing_herb"), healingHerbs,
		Item::Get("mana_herb"), manaHerbs
	};
	TerrainTile* tiles = outside->tiles;
	const Vec2 regionSize(2.f, 2.f);
	for(const ItemToSpawn& toSpawn : itemsToSpawn)
	{
		for(int i = 0; i < toSpawn.count; ++i)
		{
			for(int tries = 0; tries < 5; ++tries)
			{
				Int2 pt(Random(17, OutsideLocation::size - 17), Random(17, OutsideLocation::size - 17));
				TERRAIN_TILE type = tiles[pt.x + pt.y*OutsideLocation::size].t;
				if(type == TT_GRASS || type == TT_GRASS3)
				{
					game_level->SpawnGroundItemInsideRegion(toSpawn.item, Vec2(2.f*pt.x, 2.f*pt.y), regionSize, false);
					break;
				}
			}
		}
	}
}

//=================================================================================================
int OutsideLocationGenerator::HandleUpdate(int days)
{
	return 0;
}

//=================================================================================================
void OutsideLocationGenerator::SpawnTeam()
{
	game_level->AddPlayerTeam(team_pos, team_dir);
}

//=================================================================================================
void OutsideLocationGenerator::CreateMinimap()
{
	DynamicTexture& tex = *game->tMinimap;
	tex.Lock();

	for(int y = 0; y < OutsideLocation::size; ++y)
	{
		uint* pix = tex[y];
		for(int x = 0; x < OutsideLocation::size; ++x)
		{
			TerrainTile& t = outside->tiles[x + (OutsideLocation::size - 1 - y)*OutsideLocation::size];
			Color col;
			if(t.mode >= TM_BUILDING)
				col = Color(128, 64, 0);
			else
			{
				switch(t.t)
				{
				case TT_GRASS:
					col = Color(0, 118, 0);
					break;
				case TT_ROAD:
					col = Color(128, 128, 128);
					break;
				case TT_SAND:
					col = Color(128, 128, 64);
					break;
				case TT_GRASS2:
					col = Color(105, 128, 89);
					break;
				case TT_GRASS3:
					col = Color(32, 64, 32);
					break;
				default:
					col = Color(255, 0, 0);
					break;
				}
			}

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

	tex.Unlock();
	game_level->minimap_size = OutsideLocation::size;
}

//=================================================================================================
void OutsideLocationGenerator::OnLoad()
{
	SetOutsideParams();
	game->SetTerrainTextures();
	ApplyTiles();

	game_level->RecreateObjects(Net::IsClient());
	game_level->SpawnTerrainCollider();
	SpawnOutsideBariers();
	game->InitQuadTree();
	game->CalculateQuadtree();

	CreateMinimap();
}

//=================================================================================================
void OutsideLocationGenerator::ApplyTiles()
{
	DynamicTexture& splat = terrain->GetSplatTexture();
	splat.Lock();
	for(uint y = 0; y < 256; ++y)
	{
		uint* row = splat[y];
		for(uint x = 0; x < 256; ++x, ++row)
		{
			TerrainTile& t = outside->tiles[x / 2 + y / 2 * OutsideLocation::size];
			if(t.alpha == 0)
				*row = terrain_tile_info[t.t].mask;
			else
			{
				const TerrainTileInfo& tti1 = terrain_tile_info[t.t];
				const TerrainTileInfo& tti2 = terrain_tile_info[t.t2];
				if(tti1.shift > tti2.shift)
					*row = tti2.mask + ((255 - t.alpha) << tti1.shift);
				else
					*row = tti1.mask + (t.alpha << tti2.shift);
			}
		}
	}
	splat.Unlock(true);

	terrain->SetHeightMap(outside->h);
	terrain->Rebuild();
	terrain->CalculateBox();
}

//=================================================================================================
void OutsideLocationGenerator::SpawnOutsideBariers()
{
	LevelPart& lvlPart = *outside->lvlPart;
	const float size = 256.f;
	const float size2 = size / 2;
	const float border = 32.f;
	const float border2 = border / 2;

	// top
	{
		CollisionObject& cobj = Add1(lvlPart.colliders);
		cobj.type = CollisionObject::RECTANGLE;
		cobj.pos = Vec3(size2, 0, border2);
		cobj.w = size2;
		cobj.h = border2;

		btCollisionObject* obj = new btCollisionObject;
		obj->setCollisionShape(game_level->shape_barrier);
		obj->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_BARRIER);
		btTransform tr;
		tr.setIdentity();
		tr.setOrigin(btVector3(size2, 40.f, border2));
		obj->setWorldTransform(tr);
		phy_world->addCollisionObject(obj, CG_BARRIER);
	}

	// bottom
	{
		CollisionObject& cobj = Add1(lvlPart.colliders);
		cobj.type = CollisionObject::RECTANGLE;
		cobj.pos = Vec3(size2, 0, size - border2);
		cobj.w = size2;
		cobj.h = border2;

		btCollisionObject* obj = new btCollisionObject;
		obj->setCollisionShape(game_level->shape_barrier);
		obj->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_BARRIER);
		btTransform tr;
		tr.setIdentity();
		tr.setOrigin(btVector3(size2, 40.f, size - border2));
		obj->setWorldTransform(tr);
		phy_world->addCollisionObject(obj, CG_BARRIER);
	}

	// left
	{
		CollisionObject& cobj = Add1(lvlPart.colliders);
		cobj.type = CollisionObject::RECTANGLE;
		cobj.pos = Vec3(border2, 0, size2);
		cobj.w = border2;
		cobj.h = size2;

		btCollisionObject* obj = new btCollisionObject;
		obj->setCollisionShape(game_level->shape_barrier);
		obj->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_BARRIER);
		btTransform tr;
		tr.setOrigin(btVector3(border2, 40.f, size2));
		tr.setRotation(btQuaternion(PI / 2, 0, 0));
		obj->setWorldTransform(tr);
		phy_world->addCollisionObject(obj, CG_BARRIER);
	}

	// right
	{
		CollisionObject& cobj = Add1(lvlPart.colliders);
		cobj.type = CollisionObject::RECTANGLE;
		cobj.pos = Vec3(size - border2, 0, size2);
		cobj.w = border2;
		cobj.h = size2;

		btCollisionObject* obj = new btCollisionObject;
		obj->setCollisionShape(game_level->shape_barrier);
		obj->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_BARRIER);
		btTransform tr;
		tr.setOrigin(btVector3(size - border2, 40.f, size2));
		tr.setRotation(btQuaternion(PI / 2, 0, 0));
		obj->setWorldTransform(tr);
		phy_world->addCollisionObject(obj, CG_BARRIER);
	}
}

//=================================================================================================
void OutsideLocationGenerator::SpawnCityPhysics()
{
	TerrainTile* tiles = outside->tiles;

	for(int z = 0; z < s; ++z)
	{
		for(int x = 0; x < s; ++x)
		{
			if(tiles[x + z * OutsideLocation::size].mode == TM_BUILDING_BLOCK)
			{
				btCollisionObject* cobj = new btCollisionObject;
				cobj->setCollisionShape(game_level->shape_block);
				cobj->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_BUILDING);
				cobj->getWorldTransform().setOrigin(btVector3(2.f*x + 1.f, terrain->GetH(2.f*x + 1.f, 2.f*x + 1), 2.f*z + 1.f));
				phy_world->addCollisionObject(cobj, CG_BUILDING);
			}
		}
	}
}
