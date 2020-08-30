#include "Pch.h"
#include "Level.h"

#include "Ability.h"
#include "AIController.h"
#include "AITeam.h"
#include "Arena.h"
#include "BitStreamFunc.h"
#include "Camp.h"
#include "Chest.h"
#include "City.h"
#include "Collision.h"
#include "Door.h"
#include "Electro.h"
#include "EntityInterpolator.h"
#include "FOV.h"
#include "Game.h"
#include "GameGui.h"
#include "GameResources.h"
#include "InsideBuilding.h"
#include "InsideLocationLevel.h"
#include "Language.h"
#include "LevelGui.h"
#include "LocationGeneratorFactory.h"
#include "LocationHelper.h"
#include "MultiInsideLocation.h"
#include "Pathfinding.h"
#include "PhysicCallbacks.h"
#include "PlayerInfo.h"
#include "Portal.h"
#include "QuestManager.h"
#include "Quest_Contest.h"
#include "Quest_Scripted.h"
#include "Quest_Secret.h"
#include "Quest_Tournament.h"
#include "Quest_Tutorial.h"
#include "ScriptManager.h"
#include "Stock.h"
#include "Team.h"
#include "Trap.h"
#include "UnitEventHandler.h"
#include "UnitGroup.h"
#include "World.h"

#include <angelscript.h>
#include <ParticleSystem.h>
#include <Render.h>
#include <ResourceManager.h>
#include <Scene.h>
#include <scriptarray/scriptarray.h>
#include <SoundManager.h>
#include <Texture.h>
#include <Terrain.h>

Level* game_level;

//=================================================================================================
Level::Level() : local_area(nullptr), terrain(nullptr), terrain_shape(nullptr), dungeon_shape(nullptr), dungeon_shape_data(nullptr), shape_wall(nullptr),
shape_stairs(nullptr), shape_stairs_part(), shape_block(nullptr), shape_barrier(nullptr), shape_door(nullptr), shape_arrow(nullptr), shape_summon(nullptr),
shape_floor(nullptr), dungeon_mesh(nullptr)
{
	camera.zfar = 80.f;
	scene = new Scene;
}

//=================================================================================================
Level::~Level()
{
	DeleteElements(bow_instances);

	delete scene;
	delete terrain;
	delete terrain_shape;
	delete dungeon_mesh;
	delete dungeon_shape;
	delete dungeon_shape_data;
	delete shape_wall;
	delete shape_stairs_part[0];
	delete shape_stairs_part[1];
	delete shape_stairs;
	delete shape_block;
	delete shape_barrier;
	delete shape_door;
	delete shape_arrow;
	delete shape_summon;
	delete shape_floor;
}

//=================================================================================================
void Level::LoadLanguage()
{
	txLocationText = Str("locationText");
	txLocationTextMap = Str("locationTextMap");
	txWorldMap = Str("worldMap");
	txNewsCampCleared = Str("newsCampCleared");
	txNewsLocCleared = Str("newsLocCleared");
}

//=================================================================================================
void Level::Init()
{
	terrain = new Terrain;
	Terrain::Options terrain_options;
	terrain_options.n_parts = 8;
	terrain_options.tex_size = 256;
	terrain_options.tile_size = 2.f;
	terrain_options.tiles_per_part = 16;
	terrain->Init(terrain_options);
	terrain->Build();
	terrain->RemoveHeightMap(true);

	// collision shapes
	const float size = 256.f;
	const float border = 32.f;

	shape_wall = new btBoxShape(btVector3(1.f, 2.f, 1.f));
	btCompoundShape* s = new btCompoundShape;
	btBoxShape* b = new btBoxShape(btVector3(1.f, 2.f, 0.1f));
	shape_stairs_part[0] = b;
	btTransform tr;
	tr.setIdentity();
	tr.setOrigin(btVector3(0.f, 2.f, 0.95f));
	s->addChildShape(tr, b);
	b = new btBoxShape(btVector3(0.1f, 2.f, 1.f));
	shape_stairs_part[1] = b;
	tr.setOrigin(btVector3(-0.95f, 2.f, 0.f));
	s->addChildShape(tr, b);
	tr.setOrigin(btVector3(0.95f, 2.f, 0.f));
	s->addChildShape(tr, b);
	shape_stairs = s;
	shape_door = new btBoxShape(btVector3(Door::WIDTH, Door::HEIGHT, Door::THICKNESS));
	shape_block = new btBoxShape(btVector3(1.f, 4.f, 1.f));
	shape_barrier = new btBoxShape(btVector3(size / 2, 40.f, border / 2));
	shape_summon = new btCylinderShape(btVector3(1.5f / 2, 0.75f, 1.5f / 2));
	shape_floor = new btBoxShape(btVector3(20.f, 0.01f, 20.f));

	Mesh::Point* point = game_res->aArrow->FindPoint("Empty");
	assert(point && point->IsBox());
	shape_arrow = new btBoxShape(ToVector3(point->size));

	dungeon_mesh = new SimpleMesh;
}

//=================================================================================================
void Level::Reset()
{
	unit_warp_data.clear();
	to_remove.clear();
	minimap_reveal.clear();
	minimap_reveal_mp.clear();
}

//=================================================================================================
void Level::ProcessUnitWarps()
{
	bool warped_to_arena = false;

	for(UnitWarpData& warp : unit_warp_data)
	{
		if(warp.where == warp.unit->area->area_id)
		{
			// unit left location
			if(warp.unit->event_handler)
				warp.unit->event_handler->HandleUnitEvent(UnitEventHandler::LEAVE, warp.unit);
			RemoveUnit(warp.unit);
		}
		else if(warp.where == WARP_OUTSIDE)
		{
			// exit from building
			InsideBuilding& building = *static_cast<InsideBuilding*>(warp.unit->area);
			RemoveElement(building.units, warp.unit);
			warp.unit->area = local_area;
			if(warp.building == -1)
			{
				warp.unit->rot = building.outside_rot;
				WarpUnit(*warp.unit, building.outside_spawn);
			}
			else
			{
				CityBuilding& city_building = city_ctx->buildings[warp.building];
				WarpUnit(*warp.unit, city_building.walk_pt);
				warp.unit->RotateTo(PtToPos(city_building.pt));
			}
			local_area->units.push_back(warp.unit);
		}
		else if(warp.where == WARP_ARENA)
		{
			// warp to arena
			InsideBuilding& building = *GetArena();
			RemoveElement(warp.unit->area->units, warp.unit);
			warp.unit->area = &building;
			Vec3 pos;
			if(!WarpToRegion(building, (warp.unit->in_arena == 0 ? building.region1 : building.region2), warp.unit->GetUnitRadius(), pos, 20))
			{
				// failed to warp to arena, spawn outside of arena
				warp.unit->area = local_area;
				warp.unit->rot = building.outside_rot;
				WarpUnit(*warp.unit, building.outside_spawn);
				local_area->units.push_back(warp.unit);
				RemoveElement(game->arena->units, warp.unit);
			}
			else
			{
				warp.unit->rot = (warp.unit->in_arena == 0 ? PI : 0);
				WarpUnit(*warp.unit, pos);
				building.units.push_back(warp.unit);
				warped_to_arena = true;
			}
		}
		else
		{
			// enter building
			InsideBuilding& building = *city_ctx->inside_buildings[warp.where];
			RemoveElement(warp.unit->area->units, warp.unit);
			warp.unit->area = &building;
			warp.unit->rot = PI;
			WarpUnit(*warp.unit, building.inside_spawn);
			building.units.push_back(warp.unit);
		}

		if(warp.unit == game->pc->unit)
		{
			camera.Reset();
			game->pc->data.rot_buf = 0.f;

			if(game->fallback_type == FALLBACK::ARENA)
			{
				game->pc->unit->frozen = FROZEN::ROTATE;
				game->fallback_type = FALLBACK::ARENA2;
			}
			else if(game->fallback_type == FALLBACK::ARENA_EXIT)
			{
				game->pc->unit->frozen = FROZEN::NO;
				game->fallback_type = FALLBACK::NONE;
			}
		}
	}

	unit_warp_data.clear();

	if(warped_to_arena)
	{
		Vec3 pt1(0, 0, 0), pt2(0, 0, 0);
		int count1 = 0, count2 = 0;

		for(vector<Unit*>::iterator it = game->arena->units.begin(), end = game->arena->units.end(); it != end; ++it)
		{
			if((*it)->in_arena == 0)
			{
				pt1 += (*it)->pos;
				++count1;
			}
			else
			{
				pt2 += (*it)->pos;
				++count2;
			}
		}

		if(count1 > 0)
			pt1 /= (float)count1;
		else
		{
			InsideBuilding& building = *GetArena();
			pt1 = ((building.region1.Midpoint() + building.region2.Midpoint()) / 2).XZ();
		}
		if(count2 > 0)
			pt2 /= (float)count2;
		else
		{
			InsideBuilding& building = *GetArena();
			pt2 = ((building.region1.Midpoint() + building.region2.Midpoint()) / 2).XZ();
		}

		for(vector<Unit*>::iterator it = game->arena->units.begin(), end = game->arena->units.end(); it != end; ++it)
			(*it)->rot = Vec3::LookAtAngle((*it)->pos, (*it)->in_arena == 0 ? pt2 : pt1);
	}
}

//=================================================================================================
void Level::ProcessRemoveUnits(bool leave)
{
	if(leave)
	{
		for(Unit* unit : to_remove)
		{
			RemoveElement(unit->area->units, unit);
			delete unit;
		}
	}
	else
	{
		for(Unit* unit : to_remove)
			game->DeleteUnit(unit);
	}
	to_remove.clear();
}

//=================================================================================================
void Level::Apply()
{
	city_ctx = (location->type == L_CITY ? static_cast<City*>(location) : nullptr);
	areas.clear();
	location->Apply(areas);
	local_area = &areas[0].get();
	for(LevelArea& area : areas)
	{
		if(!area.tmp)
		{
			area.tmp = TmpLevelArea::Get();
			area.tmp->area = &area;
			area.tmp->lights_dt = 1.f;
		}
	}
}

//=================================================================================================
LevelArea& Level::GetArea(const Vec3& pos)
{
	if(!city_ctx)
		return *local_area;
	else
	{
		Int2 offset(int((pos.x - 256.f) / 256.f), int((pos.z - 256.f) / 256.f));
		if(offset.x % 2 == 1)
			++offset.x;
		if(offset.y % 2 == 1)
			++offset.y;
		offset /= 2;
		for(InsideBuilding* inside : city_ctx->inside_buildings)
		{
			if(inside->level_shift == offset)
				return *inside;
		}
		return *local_area;
	}
}

//=================================================================================================
LevelArea* Level::GetAreaById(int area_id)
{
	if(local_area->area_id == area_id)
		return local_area;
	if(city_ctx)
	{
		if(area_id >= 0 && area_id < (int)city_ctx->inside_buildings.size())
			return city_ctx->inside_buildings[area_id];
	}
	else if(LocationHelper::IsMultiLevel(location))
	{
		MultiInsideLocation* loc = static_cast<MultiInsideLocation*>(location);
		if(area_id >= 0 && area_id < (int)loc->levels.size())
			return loc->levels[area_id];
	}
	return nullptr;
}

//=================================================================================================
Unit* Level::FindUnit(int id)
{
	if(id == -1)
		return nullptr;

	for(LevelArea& area : ForEachArea())
	{
		for(Unit* unit : area.units)
		{
			if(unit->id == id)
				return unit;
		}
	}

	return nullptr;
}

//=================================================================================================
Unit* Level::FindUnit(delegate<bool(Unit*)> pred)
{
	for(LevelArea& area : ForEachArea())
	{
		for(Unit* unit : area.units)
		{
			if(pred(unit))
				return unit;
		}
	}

	return nullptr;
}

//=================================================================================================
Unit* Level::FindUnit(UnitData* ud)
{
	return FindUnit([ud](Unit* unit)
	{
		return unit->data == ud;
	});
}

//=================================================================================================
Usable* Level::FindUsable(int id)
{
	for(LevelArea& area : ForEachArea())
	{
		for(Usable* usable : area.usables)
		{
			if(usable->id == id)
				return usable;
		}
	}
	return nullptr;
}

//=================================================================================================
Door* Level::FindDoor(int id)
{
	for(LevelArea& area : ForEachArea())
	{
		for(Door* door : area.doors)
		{
			if(door->id == id)
				return door;
		}
	}

	return nullptr;
}

//=================================================================================================
Trap* Level::FindTrap(int id)
{
	for(LevelArea& area : ForEachArea())
	{
		for(Trap* trap : area.traps)
		{
			if(trap->id == id)
				return trap;
		}
	}

	return nullptr;
}

//=================================================================================================
Chest* Level::FindChest(int id)
{
	for(LevelArea& area : ForEachArea())
	{
		for(Chest* chest : area.chests)
		{
			if(chest->id == id)
				return chest;
		}
	}

	return nullptr;
}

//=================================================================================================
Chest* Level::GetRandomChest(Room& room)
{
	LocalVector<Chest*> chests;
	for(Chest* chest : local_area->chests)
	{
		if(room.IsInside(chest->pos))
			chests.push_back(chest);
	}
	if(chests.empty())
		return nullptr;
	return chests[Rand() % chests.size()];
}

//=================================================================================================
Chest* Level::GetTreasureChest()
{
	assert(lvl);

	Room* room;
	if(location->target == LABYRINTH)
		room = lvl->rooms[0];
	else
		room = lvl->rooms[static_cast<InsideLocation*>(location)->special_room];

	Chest* bestChest = nullptr;
	const Vec3 center = room->Center();
	float bestDist = 999.f;
	for(Chest* chest : lvl->chests)
	{
		if(room->IsInside(chest->pos))
		{
			float dist = Vec3::Distance(chest->pos, center);
			if(dist < bestDist)
			{
				bestDist = dist;
				bestChest = chest;
			}
		}
	}

	return bestChest;
}

//=================================================================================================
Electro* Level::FindElectro(int id)
{
	for(LevelArea& area : ForEachArea())
	{
		for(Electro* electro : area.tmp->electros)
		{
			if(electro->id == id)
				return electro;
		}
	}

	return nullptr;
}

//=================================================================================================
bool Level::RemoveTrap(int id)
{
	for(LevelArea& area : ForEachArea())
	{
		for(vector<Trap*>::iterator it = area.traps.begin(), end = area.traps.end(); it != end; ++it)
		{
			if((*it)->id == id)
			{
				delete *it;
				area.traps.erase(it);
				return true;
			}
		}
	}

	return false;
}

//=================================================================================================
void Level::RemoveUnit(Unit* unit, bool notify)
{
	assert(unit);
	if(unit->action == A_DESPAWN || (Net::IsClient() && unit->summoner))
		SpawnUnitEffect(*unit);
	unit->RemoveAllEventHandlers();
	unit->to_remove = true;
	to_remove.push_back(unit);
	if(notify && Net::IsServer())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::REMOVE_UNIT;
		c.id = unit->id;
	}
}

//=================================================================================================
void Level::RemoveUnit(UnitData* ud, bool on_leave)
{
	assert(ud);

	Unit* unit = FindUnit([=](Unit* unit)
	{
		return unit->data == ud && unit->IsAlive();
	});

	if(unit)
		RemoveUnit(unit, !on_leave);
}

//=================================================================================================
ObjectEntity Level::SpawnObjectEntity(LevelArea& area, BaseObject* base, const Vec3& pos, float rot, float scale, int flags, Vec3* out_point, int variant)
{
	if(IsSet(base->flags, OBJ_TABLE_SPAWNER))
	{
		// table & stools
		BaseObject* table = BaseObject::Get(Rand() % 2 == 0 ? "table" : "table2");
		BaseUsable* stool = BaseUsable::Get("stool");

		// table
		Object* o = new Object;
		o->mesh = table->mesh;
		o->rot = Vec3(0, rot, 0);
		o->pos = pos;
		o->scale = 1;
		o->base = table;
		area.objects.push_back(o);
		SpawnObjectExtras(area, table, pos, rot, o);

		// stools
		int count = Random(2, 4);
		int d[4] = { 0,1,2,3 };
		for(int i = 0; i < 4; ++i)
			std::swap(d[Rand() % 4], d[Rand() % 4]);

		for(int i = 0; i < count; ++i)
		{
			float sdir, slen;
			switch(d[i])
			{
			case 0:
				sdir = 0.f;
				slen = table->size.y + 0.3f;
				break;
			case 1:
				sdir = PI / 2;
				slen = table->size.x + 0.3f;
				break;
			case 2:
				sdir = PI;
				slen = table->size.y + 0.3f;
				break;
			case 3:
				sdir = PI * 3 / 2;
				slen = table->size.x + 0.3f;
				break;
			default:
				assert(0);
				break;
			}

			sdir += rot;

			Usable* u = new Usable;
			u->Register();
			u->base = stool;
			u->pos = pos + Vec3(sin(sdir) * slen, 0, cos(sdir) * slen);
			u->rot = sdir;
			area.usables.push_back(u);

			SpawnObjectExtras(area, stool, u->pos, u->rot, u);
		}

		return o;
	}
	else if(IsSet(base->flags, OBJ_BUILDING))
	{
		// building
		Object* o = new Object;
		o->mesh = base->mesh;
		o->rot = Vec3(0, rot, 0);
		o->pos = pos;
		o->scale = scale;
		o->base = base;
		area.objects.push_back(o);

		const GameDirection dir = RotToDir(rot);
		ProcessBuildingObjects(area, nullptr, nullptr, o->mesh, nullptr, rot, dir, pos, nullptr, nullptr, false, out_point);

		return o;
	}
	else if(IsSet(base->flags, OBJ_USABLE))
	{
		// usable object
		BaseUsable* base_use = (BaseUsable*)base;

		Usable* u = new Usable;
		u->Register();
		u->base = base_use;
		u->pos = pos;
		u->rot = rot;

		if(IsSet(base_use->use_flags, BaseUsable::CONTAINER))
		{
			u->container = new ItemContainer;
			const Item* item = Book::GetRandom();
			if(item)
				u->container->items.push_back({ item, 1, 1 });
			if(Rand() % 2 == 0)
			{
				uint level;
				if(city_ctx)
					level = 1;
				else
					level = GetChestDifficultyLevel();
				uint gold = Random(level * 5, level * 10);
				u->container->items.push_back({ Item::gold, gold , gold });
			}
		}

		if(variant == -1)
		{
			if(base->variants)
			{
				// extra code for bench
				if(IsSet(base_use->use_flags, BaseUsable::IS_BENCH))
				{
					switch(location->type)
					{
					case L_CITY:
						variant = 0;
						break;
					case L_DUNGEON:
						variant = Rand() % 2;
						break;
					default:
						variant = Rand() % 2 + 2;
						break;
					}
				}
				else
					variant = Random<int>(base->variants->meshes.size() - 1);
			}
		}
		u->variant = variant;

		area.usables.push_back(u);

		SpawnObjectExtras(area, base, pos, rot, u, scale, flags);

		return u;
	}
	else if(IsSet(base->flags, OBJ_IS_CHEST))
	{
		// chest
		Chest* chest = new Chest;
		chest->Register();
		chest->base = base;
		chest->rot = rot;
		chest->pos = pos;
		area.chests.push_back(chest);

		SpawnObjectExtras(area, base, pos, rot, nullptr, scale, flags);

		return chest;
	}
	else
	{
		// normal object
		Object* o = new Object;
		o->mesh = base->mesh;
		o->rot = Vec3(0, rot, 0);
		o->pos = pos;
		o->scale = scale;
		o->base = base;
		area.objects.push_back(o);

		SpawnObjectExtras(area, base, pos, rot, o, scale, flags);

		return o;
	}
}

//=================================================================================================
void Level::SpawnObjectExtras(LevelArea& area, BaseObject* obj, const Vec3& pos, float rot, void* user_ptr, float scale, int flags)
{
	assert(obj);

	// ogie� pochodni
	if(!IsSet(flags, SOE_DONT_SPAWN_PARTICLES))
	{
		if(IsSet(obj->flags, OBJ_LIGHT))
		{
			ParticleEmitter* pe = new ParticleEmitter;
			pe->alpha = 0.8f;
			pe->emision_interval = 0.1f;
			pe->emisions = -1;
			pe->life = -1;
			pe->max_particles = 50;
			pe->op_alpha = ParticleEmitter::POP_LINEAR_SHRINK;
			pe->op_size = ParticleEmitter::POP_LINEAR_SHRINK;
			pe->particle_life = 0.5f;
			pe->pos = pos;
			pe->pos.y += obj->centery;
			pe->pos_min = Vec3(0, 0, 0);
			pe->pos_max = Vec3(0, 0, 0);
			pe->spawn_min = 1;
			pe->spawn_max = 3;
			pe->speed_min = Vec3(-1, 3, -1);
			pe->speed_max = Vec3(1, 4, 1);
			pe->mode = 1;
			pe->Init();
			area.tmp->pes.push_back(pe);

			pe->tex = game_res->tFlare;
			if(IsSet(obj->flags, OBJ_CAMPFIRE_EFFECT))
				pe->size = 0.7f;
			else
			{
				pe->size = 0.5f;
				if(IsSet(flags, SOE_MAGIC_LIGHT))
					pe->tex = game_res->tFlare2;
			}

			// �wiat�o
			if(!IsSet(flags, SOE_DONT_CREATE_LIGHT))
			{
				GameLight& light = Add1(area.lights);
				light.start_pos = pe->pos;
				light.range = 5;
				if(IsSet(flags, SOE_MAGIC_LIGHT))
					light.start_color = Vec3(0.8f, 0.8f, 1.f);
				else
					light.start_color = Vec3(1.f, 0.9f, 0.9f);
			}
		}
		else if(IsSet(obj->flags, OBJ_BLOOD_EFFECT))
		{
			// krew
			ParticleEmitter* pe = new ParticleEmitter;
			pe->alpha = 0.8f;
			pe->emision_interval = 0.1f;
			pe->emisions = -1;
			pe->life = -1;
			pe->max_particles = 50;
			pe->op_alpha = ParticleEmitter::POP_LINEAR_SHRINK;
			pe->op_size = ParticleEmitter::POP_LINEAR_SHRINK;
			pe->particle_life = 0.5f;
			pe->pos = pos;
			pe->pos.y += obj->centery;
			pe->pos_min = Vec3(0, 0, 0);
			pe->pos_max = Vec3(0, 0, 0);
			pe->spawn_min = 1;
			pe->spawn_max = 3;
			pe->speed_min = Vec3(-1, 4, -1);
			pe->speed_max = Vec3(1, 6, 1);
			pe->mode = 0;
			pe->tex = game_res->tBlood[BLOOD_RED];
			pe->size = 0.5f;
			pe->Init();
			area.tmp->pes.push_back(pe);
		}
		else if(IsSet(obj->flags, OBJ_WATER_EFFECT))
		{
			// krew
			ParticleEmitter* pe = new ParticleEmitter;
			pe->alpha = 0.8f;
			pe->emision_interval = 0.1f;
			pe->emisions = -1;
			pe->life = -1;
			pe->max_particles = 500;
			pe->op_alpha = ParticleEmitter::POP_LINEAR_SHRINK;
			pe->op_size = ParticleEmitter::POP_LINEAR_SHRINK;
			pe->particle_life = 3.f;
			pe->pos = pos;
			pe->pos.y += obj->centery;
			pe->pos_min = Vec3(0, 0, 0);
			pe->pos_max = Vec3(0, 0, 0);
			pe->spawn_min = 4;
			pe->spawn_max = 8;
			pe->speed_min = Vec3(-0.6f, 4, -0.6f);
			pe->speed_max = Vec3(0.6f, 7, 0.6f);
			pe->mode = 0;
			pe->tex = game_res->tWater;
			pe->size = 0.05f;
			pe->Init();
			area.tmp->pes.push_back(pe);
		}
	}

	// fizyka
	if(obj->shape)
	{
		CollisionObject& c = Add1(area.tmp->colliders);
		c.owner = user_ptr;
		c.cam_collider = IsSet(obj->flags, OBJ_PHY_BLOCKS_CAM);

		int group = CG_OBJECT;
		if(IsSet(obj->flags, OBJ_PHY_BLOCKS_CAM))
			group |= CG_CAMERA_COLLIDER;

		btCollisionObject* cobj = new btCollisionObject;
		cobj->setCollisionShape(obj->shape);
		cobj->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | group);

		if(obj->type == OBJ_CYLINDER)
		{
			cobj->getWorldTransform().setOrigin(btVector3(pos.x, pos.y + obj->h / 2, pos.z));
			c.type = CollisionObject::SPHERE;
			c.pos = pos;
			c.radius = obj->r;
		}
		else if(obj->type == OBJ_HITBOX)
		{
			btTransform& tr = cobj->getWorldTransform();
			const Matrix mat = *obj->matrix * Matrix::RotationY(rot);
			Vec3 pos2 = Vec3::TransformZero(mat);
			pos2 += pos;
			tr.setOrigin(ToVector3(pos2));
			tr.setRotation(btQuaternion(rot, 0, 0));

			c.pos = pos2;
			c.w = obj->size.x;
			c.h = obj->size.y;
			if(NotZero(rot))
			{
				c.type = CollisionObject::RECTANGLE_ROT;
				c.rot = rot;
				c.radius = max(c.w, c.h) * SQRT_2;
			}
			else
				c.type = CollisionObject::RECTANGLE;
		}
		else
		{
			const Matrix m_rot = Matrix::RotationY(rot);
			const Matrix m_pos = Matrix::Translation(pos);
			// skalowanie jakim� sposobem przechodzi do btWorldTransform i przy rysowaniu jest z�a skala (dwukrotnie u�yta)
			const Matrix m_scale = Matrix::Scale(1.f / obj->size.x, 1.f, 1.f / obj->size.y);
			const Matrix m = m_scale * *obj->matrix * m_rot * m_pos;
			cobj->getWorldTransform().setFromOpenGLMatrix(&m._11);
			Vec3 out_pos = Vec3::TransformZero(m);
			Quat q = Quat::CreateFromRotationMatrix(m);

			float yaw = asin(-2 * (q.x * q.z - q.w * q.y));
			c.pos = out_pos;
			c.w = obj->size.x;
			c.h = obj->size.y;
			if(NotZero(yaw))
			{
				c.type = CollisionObject::RECTANGLE_ROT;
				c.rot = yaw;
				c.radius = max(c.w, c.h) * SQRT_2;
			}
			else
				c.type = CollisionObject::RECTANGLE;
		}

		phy_world->addCollisionObject(cobj, group);

		if(IsSet(obj->flags, OBJ_PHYSICS_PTR))
		{
			assert(user_ptr);
			cobj->setUserPointer(user_ptr);
		}

		if(IsSet(obj->flags, OBJ_DOUBLE_PHYSICS))
			SpawnObjectExtras(area, obj->next_obj, pos, rot, user_ptr, scale, flags);
		else if(IsSet(obj->flags, OBJ_MULTI_PHYSICS))
		{
			for(int i = 0;; ++i)
			{
				if(obj->next_obj[i].shape)
					SpawnObjectExtras(area, &obj->next_obj[i], pos, rot, user_ptr, scale, flags);
				else
					break;
			}
		}
	}
	else if(IsSet(obj->flags, OBJ_SCALEABLE))
	{
		CollisionObject& c = Add1(area.tmp->colliders);
		c.type = CollisionObject::SPHERE;
		c.pos = pos;
		c.radius = obj->r * scale;

		btCollisionObject* cobj = new btCollisionObject;
		btCylinderShape* shape = new btCylinderShape(btVector3(obj->r * scale, obj->h * scale, obj->r * scale));
		shapes.push_back(shape);
		cobj->setCollisionShape(shape);
		cobj->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_OBJECT);
		cobj->getWorldTransform().setOrigin(btVector3(pos.x, pos.y + obj->h / 2 * scale, pos.z));
		phy_world->addCollisionObject(cobj, CG_OBJECT);
	}
	else if(IsSet(obj->flags, OBJ_TMP_PHYSICS))
	{
		CollisionObject& c = Add1(area.tmp->colliders);
		c.owner = CollisionObject::TMP;
		c.cam_collider = false;
		if(obj->type == OBJ_CYLINDER)
		{
			c.type = CollisionObject::SPHERE;
			c.pos = pos;
			c.radius = obj->r;
		}
		else if(obj->type == OBJ_HITBOX)
		{
			const Matrix mat = *obj->matrix * Matrix::RotationY(rot);
			Vec3 pos2 = Vec3::TransformZero(mat);
			pos2 += pos;

			c.pos = pos2;
			c.w = obj->size.x;
			c.h = obj->size.y;
			if(NotZero(rot))
			{
				c.type = CollisionObject::RECTANGLE_ROT;
				c.rot = rot;
				c.radius = max(c.w, c.h) * SQRT_2;
			}
			else
				c.type = CollisionObject::RECTANGLE;
		}
	}

	if(IsSet(obj->flags, OBJ_CAM_COLLIDERS))
	{
		int roti = (int)round((rot / (PI / 2)));
		for(vector<Mesh::Point>::const_iterator it = obj->mesh->attach_points.begin(), end = obj->mesh->attach_points.end(); it != end; ++it)
		{
			const Mesh::Point& pt = *it;
			if(strncmp(pt.name.c_str(), "camcol", 6) != 0)
				continue;

			const Matrix m = pt.mat * Matrix::RotationY(rot);
			Vec3 pos2 = Vec3::TransformZero(m) + pos;

			btBoxShape* shape = new btBoxShape(btVector3(pt.size.x, pt.size.y, pt.size.z));
			shapes.push_back(shape);
			btCollisionObject* co = new btCollisionObject;
			co->setCollisionShape(shape);
			co->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_CAMERA_COLLIDER);
			co->getWorldTransform().setOrigin(ToVector3(pos2));
			phy_world->addCollisionObject(co, CG_CAMERA_COLLIDER);
			if(roti != 0)
				co->getWorldTransform().setRotation(btQuaternion(rot, 0, 0));

			float w = pt.size.x, h = pt.size.z;
			if(roti == 1 || roti == 3)
				std::swap(w, h);

			CameraCollider& cc = Add1(cam_colliders);
			cc.box.v1.x = pos2.x - w;
			cc.box.v2.x = pos2.x + w;
			cc.box.v1.z = pos2.z - h;
			cc.box.v2.z = pos2.z + h;
			cc.box.v1.y = pos2.y - pt.size.y;
			cc.box.v2.y = pos2.y + pt.size.y;
		}
	}
}

//=================================================================================================
void Level::ProcessBuildingObjects(LevelArea& area, City* city, InsideBuilding* inside, Mesh* mesh, Mesh* inside_mesh, float rot, GameDirection dir,
	const Vec3& shift, Building* building, CityBuilding* city_building, bool recreate, Vec3* out_point)
{
	if(mesh->attach_points.empty())
	{
		city_building->walk_pt = shift;
		assert(!inside);
		return;
	}

	// https://github.com/Tomash667/carpg/wiki/%5BPL%5D-Buildings-export
	// o_x_[!N!]nazwa_???
	// x (o - obiekt, r - obr�cony obiekt, p - fizyka, s - strefa, c - posta�, m - maska �wiat�a, d - detal wok� obiektu, l - limited rot object)
	// N - wariant (tylko obiekty)
	string token;
	bool have_exit = false, have_spawn = false;
	bool is_inside = (inside != nullptr);
	Vec3 spawn_point;
	static vector<const Mesh::Point*> details;

	for(vector<Mesh::Point>::const_iterator it2 = mesh->attach_points.begin(), end2 = mesh->attach_points.end(); it2 != end2; ++it2)
	{
		const Mesh::Point& pt = *it2;
		if(pt.name.length() < 5 || pt.name[0] != 'o')
			continue;

		char c = pt.name[2];
		if(c == 'o' || c == 'r' || c == 'p' || c == 's' || c == 'c' || c == 'l' || c == 'e')
		{
			uint poss = pt.name.find_first_of('_', 4);
			if(poss == string::npos)
				poss = pt.name.length();
			token = pt.name.substr(4, poss - 4);
			for(uint k = 0, len = token.length(); k < len; ++k)
			{
				if(token[k] == '#')
					token[k] = '_';
			}
		}
		else if(c == 'm')
		{
			// light mask
			// nothing to do here
		}
		else if(c == 'd')
		{
			if(!recreate)
			{
				assert(!is_inside);
				details.push_back(&pt);
			}
			continue;
		}
		else
			continue;

		Vec3 pos;
		if(dir != GDIR_DOWN)
		{
			const Matrix m = pt.mat * Matrix::RotationY(rot);
			pos = Vec3::TransformZero(m);
		}
		else
			pos = Vec3::TransformZero(pt.mat);
		const float height = pos.y;
		pos += shift;

		switch(c)
		{
		case 'o': // object
		case 'r': // rotated object
		case 'l': // random rotation (90* angle) object
			if(!recreate)
			{
				cstring name;
				int variant = -1;
				if(token[0] == '!')
				{
					// p�ki co tylko 0-9
					variant = int(token[1] - '0');
					assert(InRange(variant, 0, 9));
					assert(token[2] == '!');
					name = token.c_str() + 3;
				}
				else
					name = token.c_str();

				BaseObject* base = BaseObject::TryGet(name);
				assert(base);

				if(base)
				{
					if(area.area_type == LevelArea::Type::Outside)
						pos.y = terrain->GetH(pos) + height;

					float r;
					switch(c)
					{
					case 'o':
					default:
						r = Random(MAX_ANGLE);
						break;
					case 'r':
						r = Clip(pt.rot.y + rot);
						break;
					case 'l':
						r = PI / 2 * (Rand() % 4);
						break;
					}
					SpawnObjectEntity(area, base, pos, r, 1.f, 0, nullptr, variant);
				}
			}
			break;
		case 'p': // physics
			if(token == "circle" || token == "circlev")
			{
				bool is_wall = (token == "circle");

				CollisionObject& cobj = Add1(area.tmp->colliders);
				cobj.type = CollisionObject::SPHERE;
				cobj.radius = pt.size.x;
				cobj.pos = pos;
				cobj.owner = nullptr;
				cobj.cam_collider = is_wall;

				if(area.area_type == LevelArea::Type::Outside)
				{
					terrain->SetY(pos);
					pos.y += 2.f;
				}

				btCylinderShape* shape = new btCylinderShape(btVector3(pt.size.x, 4.f, pt.size.z));
				shapes.push_back(shape);
				btCollisionObject* co = new btCollisionObject;
				co->setCollisionShape(shape);
				int group = (is_wall ? CG_BUILDING : CG_COLLIDER);
				co->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | group);
				co->getWorldTransform().setOrigin(ToVector3(pos));
				phy_world->addCollisionObject(co, group);
			}
			else if(token == "square" || token == "squarev" || token == "squarevn" || token == "squarevp")
			{
				bool is_wall = (token == "square" || token == "squarevn");
				bool block_camera = (token == "square");

				CollisionObject& cobj = Add1(area.tmp->colliders);
				cobj.type = CollisionObject::RECTANGLE;
				cobj.pos = pos;
				cobj.w = pt.size.x;
				cobj.h = pt.size.z;
				cobj.owner = nullptr;
				cobj.cam_collider = block_camera;

				btBoxShape* shape;
				if(token != "squarevp")
				{
					shape = new btBoxShape(btVector3(pt.size.x, 16.f, pt.size.z));
					if(area.area_type == LevelArea::Type::Outside)
					{
						terrain->SetY(pos);
						pos.y += 8.f;
					}
					else
						pos.y = 0.f;
				}
				else
				{
					shape = new btBoxShape(btVector3(pt.size.x, pt.size.y, pt.size.z));
					if(area.area_type == LevelArea::Type::Outside)
						pos.y += terrain->GetH(pos);
				}
				shapes.push_back(shape);
				btCollisionObject* co = new btCollisionObject;
				co->setCollisionShape(shape);
				int group = (is_wall ? CG_BUILDING : CG_COLLIDER);
				co->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | group);
				co->getWorldTransform().setOrigin(ToVector3(pos));
				phy_world->addCollisionObject(co, group);

				if(dir != GDIR_DOWN)
				{
					cobj.type = CollisionObject::RECTANGLE_ROT;
					cobj.rot = rot;
					cobj.radius = sqrt(cobj.w * cobj.w + cobj.h * cobj.h);
					co->getWorldTransform().setRotation(btQuaternion(rot, 0, 0));
				}
			}
			else if(token == "squarevpa")
			{
				btBoxShape* shape = new btBoxShape(btVector3(pt.size.x, pt.size.y, pt.size.z));
				if(area.area_type == LevelArea::Type::Outside)
					pos.y += terrain->GetH(pos);
				shapes.push_back(shape);
				btCollisionObject* co = new btCollisionObject;
				co->setCollisionShape(shape);
				int group = CG_COLLIDER;
				co->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | group);
				co->getWorldTransform().setOrigin(ToVector3(pos));
				phy_world->addCollisionObject(co, group);

				if(dir != GDIR_DOWN)
					co->getWorldTransform().setRotation(btQuaternion(rot, 0, 0));
			}
			else if(token == "squarecam")
			{
				if(area.area_type == LevelArea::Type::Outside)
					pos.y += terrain->GetH(pos);

				btBoxShape* shape = new btBoxShape(btVector3(pt.size.x, pt.size.y, pt.size.z));
				shapes.push_back(shape);
				btCollisionObject* co = new btCollisionObject;
				co->setCollisionShape(shape);
				co->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_CAMERA_COLLIDER);
				co->getWorldTransform().setOrigin(ToVector3(pos));
				phy_world->addCollisionObject(co, CG_CAMERA_COLLIDER);
				if(dir != GDIR_DOWN)
					co->getWorldTransform().setRotation(btQuaternion(rot, 0, 0));

				float w = pt.size.x, h = pt.size.z;
				if(dir == GDIR_LEFT || dir == GDIR_RIGHT)
					std::swap(w, h);

				CameraCollider& cc = Add1(cam_colliders);
				cc.box.v1.x = pos.x - w;
				cc.box.v2.x = pos.x + w;
				cc.box.v1.z = pos.z - h;
				cc.box.v2.z = pos.z + h;
				cc.box.v1.y = pos.y - pt.size.y;
				cc.box.v2.y = pos.y + pt.size.y;
			}
			else if(token == "xsphere")
			{
				inside->xsphere_pos = pos;
				inside->xsphere_radius = pt.size.x;
			}
			else
				assert(0);
			break;
		case 's': // zone
			if(!recreate)
			{
				if(token == "enter")
				{
					assert(!inside);

					inside = new InsideBuilding((int)city->inside_buildings.size());
					inside->tmp = TmpLevelArea::Get();
					inside->tmp->area = inside;
					inside->tmp->lights_dt = 1.f;
					inside->level_shift = city->inside_offset;
					inside->offset = Vec2(512.f * city->inside_offset.x + 256.f, 512.f * city->inside_offset.y + 256.f);
					if(city->inside_offset.x > city->inside_offset.y)
					{
						--city->inside_offset.x;
						++city->inside_offset.y;
					}
					else
					{
						city->inside_offset.x += 2;
						city->inside_offset.y = 0;
					}
					float w, h;
					if(dir == GDIR_DOWN || dir == GDIR_UP)
					{
						w = pt.size.x;
						h = pt.size.z;
					}
					else
					{
						w = pt.size.z;
						h = pt.size.x;
					}
					inside->enter_region.v1.x = pos.x - w;
					inside->enter_region.v1.y = pos.z - h;
					inside->enter_region.v2.x = pos.x + w;
					inside->enter_region.v2.y = pos.z + h;
					Vec2 mid = inside->enter_region.Midpoint();
					inside->enter_y = terrain->GetH(mid.x, mid.y) + 0.1f;
					inside->building = building;
					inside->outside_rot = rot;
					inside->top = -1.f;
					inside->xsphere_radius = -1.f;
					city->inside_buildings.push_back(inside);
					areas.push_back(*inside);

					assert(inside_mesh);

					if(inside_mesh)
					{
						Vec3 o_pos = Vec3(inside->offset.x, 0.f, inside->offset.y);

						Object* o = new Object;
						o->base = nullptr;
						o->mesh = inside_mesh;
						o->pos = o_pos;
						o->rot = Vec3(0, 0, 0);
						o->scale = 1.f;
						o->requireSplit = true;
						inside->objects.push_back(o);

						ProcessBuildingObjects(*inside, city, inside, inside_mesh, nullptr, 0.f, GDIR_DOWN, o->pos, nullptr, nullptr);
					}

					have_exit = true;
				}
				else if(token == "exit")
				{
					assert(inside);

					inside->exit_region.v1.x = pos.x - pt.size.x;
					inside->exit_region.v1.y = pos.z - pt.size.z;
					inside->exit_region.v2.x = pos.x + pt.size.x;
					inside->exit_region.v2.y = pos.z + pt.size.z;

					have_exit = true;
				}
				else if(token == "spawn")
				{
					if(is_inside)
						inside->inside_spawn = pos;
					else
					{
						spawn_point = pos;
						terrain->SetY(spawn_point);
					}

					have_spawn = true;
				}
				else if(token == "top")
				{
					assert(is_inside);
					inside->top = pos.y;
				}
				else if(token == "door" || token == "doorc" || token == "doorl" || token == "door2")
				{
					Door* door = new Door;
					door->pos = pos;
					door->rot = Clip(pt.rot.y + rot);
					door->locked = LOCK_NONE;
					if(token != "door") // door2 are closed now, this is intended
					{
						door->state = Door::Closed;
						if(token == "doorl")
							door->locked = LOCK_UNLOCKABLE;
					}
					else
						door->state = Door::Opened;
					door->door2 = (token == "door2");
					door->Init();
					area.doors.push_back(door);
				}
				else if(token == "arena")
				{
					assert(inside);

					inside->region1.v1.x = pos.x - pt.size.x;
					inside->region1.v1.y = pos.z - pt.size.z;
					inside->region1.v2.x = pos.x + pt.size.x;
					inside->region1.v2.y = pos.z + pt.size.z;
				}
				else if(token == "arena2")
				{
					assert(inside);

					inside->region2.v1.x = pos.x - pt.size.x;
					inside->region2.v1.y = pos.z - pt.size.z;
					inside->region2.v2.x = pos.x + pt.size.x;
					inside->region2.v2.y = pos.z + pt.size.z;
				}
				else if(token == "viewer")
				{
					// ten punkt jest u�ywany w SpawnArenaViewers
				}
				else if(token == "point")
				{
					if(city_building)
					{
						city_building->walk_pt = pos;
						terrain->SetY(city_building->walk_pt);
					}
					else if(out_point)
						*out_point = pos;
				}
				else
					assert(0);
			}
			break;
		case 'c': // unit
			if(!recreate)
			{
				UnitData* ud = UnitData::TryGet(token.c_str());
				assert(ud);
				if(ud)
				{
					Unit* u = SpawnUnitNearLocation(area, pos, *ud, nullptr, -2);
					if(u)
					{
						u->rot = Clip(pt.rot.y + rot);
						u->ai->start_rot = u->rot;
					}
				}
			}
			break;
		case 'm': // light mask
			{
				LightMask& mask = Add1(inside->masks);
				mask.size = Vec2(pt.size.x, pt.size.z);
				mask.pos = Vec2(pos.x, pos.z);
			}
			break;
		case 'e': // effect
			if(token == "magicfire")
			{
				if(!game->in_load)
				{
					ParticleEmitter* pe = new ParticleEmitter;
					pe->tex = game_res->tFlare2;
					pe->alpha = 1.0f;
					pe->size = 1.0f;
					pe->emision_interval = 0.1f;
					pe->emisions = -1;
					pe->life = -1;
					pe->max_particles = 50;
					pe->op_alpha = ParticleEmitter::POP_LINEAR_SHRINK;
					pe->op_size = ParticleEmitter::POP_LINEAR_SHRINK;
					pe->particle_life = 0.5f;
					pe->pos = pos;
					if(area.area_type == LevelArea::Type::Outside)
						pe->pos.y += terrain->GetH(pos);
					pe->pos_min = Vec3(0, 0, 0);
					pe->pos_max = Vec3(0, 0, 0);
					pe->spawn_min = 2;
					pe->spawn_max = 4;
					pe->speed_min = Vec3(-1, 3, -1);
					pe->speed_max = Vec3(1, 4, 1);
					pe->mode = 1;
					pe->Init();
					area.tmp->pes.push_back(pe);
				}
			}
			else
				assert(0);
			break;
		}
	}

	if(is_inside)
	{
		// floor
		btCollisionObject* co = new btCollisionObject;
		co->setCollisionShape(shape_floor);
		co->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_COLLIDER);
		co->getWorldTransform().setOrigin(ToVector3(shift));
		phy_world->addCollisionObject(co, CG_COLLIDER);

		// ceiling
		co = new btCollisionObject;
		co->setCollisionShape(shape_floor);
		co->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_COLLIDER);
		co->getWorldTransform().setOrigin(ToVector3(shift + Vec3(0, 4, 0)));
		phy_world->addCollisionObject(co, CG_COLLIDER);
	}

	if(!details.empty() && !is_inside)
	{
		int c = Rand() % 80 + details.size() * 2, count;
		if(c < 10)
			count = 0;
		else if(c < 30)
			count = 1;
		else if(c < 60)
			count = 2;
		else if(c < 90)
			count = 3;
		else
			count = 4;
		if(count > 0)
		{
			Shuffle(details.begin(), details.end());
			while(count && !details.empty())
			{
				const Mesh::Point& pt = *details.back();
				details.pop_back();
				--count;

				uint poss = pt.name.find_first_of('_', 4);
				if(poss == string::npos)
				{
					assert(0);
					continue;
				}
				token = pt.name.substr(4, poss - 4);
				for(uint k = 0, len = token.length(); k < len; ++k)
				{
					if(token[k] == '#')
						token[k] = '_';
				}

				Vec3 pos;
				if(dir != GDIR_DOWN)
				{
					const Matrix m = pt.mat * Matrix::RotationY(rot);
					pos = Vec3::TransformZero(m);
				}
				else
					pos = Vec3::TransformZero(pt.mat);
				pos += shift;

				cstring name;
				int variant = -1;
				if(token[0] == '!')
				{
					// p�ki co tylko 0-9
					variant = int(token[1] - '0');
					assert(InRange(variant, 0, 9));
					assert(token[2] == '!');
					name = token.c_str() + 3;
				}
				else
					name = token.c_str();

				BaseObject* obj = BaseObject::TryGet(name);
				assert(obj);

				if(obj)
				{
					if(area.area_type == LevelArea::Type::Outside)
						terrain->SetY(pos);
					SpawnObjectEntity(area, obj, pos, Clip(pt.rot.y + rot), 1.f, 0, nullptr, variant);
				}
			}
		}
		details.clear();
	}

	if(!recreate)
	{
		if(is_inside || inside)
			assert(have_exit && have_spawn);

		if(!is_inside && inside)
			inside->outside_spawn = spawn_point;
	}
}

//=================================================================================================
void Level::RecreateObjects(bool spawn_pes)
{
	static BaseObject* chest = BaseObject::Get("chest");

	for(LevelArea& area : ForEachArea())
	{
		int flags = Level::SOE_DONT_CREATE_LIGHT;
		if(!spawn_pes)
			flags |= Level::SOE_DONT_SPAWN_PARTICLES;

		// dotyczy tylko pochodni
		if(area.area_type == LevelArea::Type::Inside)
		{
			InsideLocation* inside = (InsideLocation*)location;
			BaseLocation& base = g_base_locations[inside->target];
			if(IsSet(base.options, BLO_MAGIC_LIGHT))
				flags |= Level::SOE_MAGIC_LIGHT;
		}

		for(vector<Object*>::iterator it = area.objects.begin(), end = area.objects.end(); it != end; ++it)
		{
			Object& obj = **it;
			BaseObject* base_obj = obj.base;

			if(!base_obj)
				continue;

			if(IsSet(base_obj->flags, OBJ_BUILDING))
			{
				const float rot = obj.rot.y;
				const GameDirection dir = RotToDir(rot);
				ProcessBuildingObjects(area, nullptr, nullptr, base_obj->mesh, nullptr, rot, dir, obj.pos, nullptr, nullptr, true);
			}
			else
				SpawnObjectExtras(area, base_obj, obj.pos, obj.rot.y, &obj, obj.scale, flags);
		}

		for(vector<Chest*>::iterator it = area.chests.begin(), end = area.chests.end(); it != end; ++it)
			SpawnObjectExtras(area, chest, (*it)->pos, (*it)->rot, nullptr, 1.f, flags);

		for(vector<Usable*>::iterator it = area.usables.begin(), end = area.usables.end(); it != end; ++it)
			SpawnObjectExtras(area, (*it)->base, (*it)->pos, (*it)->rot, *it, 1.f, flags);
	}
}

//=================================================================================================
ObjectEntity Level::SpawnObjectNearLocation(LevelArea& area, BaseObject* obj, const Vec2& pos, float rot, float range, float margin, float scale)
{
	bool ok = false;
	if(obj->type == OBJ_CYLINDER)
	{
		global_col.clear();
		Vec3 pt(pos.x, 0, pos.y);
		GatherCollisionObjects(area, global_col, pt, obj->r + margin + range);
		float extra_radius = range / 20;
		for(int i = 0; i < 20; ++i)
		{
			if(!Collide(global_col, pt, obj->r + margin))
			{
				ok = true;
				break;
			}
			pt = Vec3(pos.x, 0, pos.y);
			pt += Vec2::RandomPoissonDiscPoint().XZ() * extra_radius;
			extra_radius += range / 20;
		}

		if(!ok)
			return nullptr;

		if(area.area_type == LevelArea::Type::Outside)
			terrain->SetY(pt);

		return SpawnObjectEntity(area, obj, pt, rot, scale);
	}
	else
	{
		global_col.clear();
		Vec3 pt(pos.x, 0, pos.y);
		GatherCollisionObjects(area, global_col, pt, sqrt(obj->size.x + obj->size.y) + margin + range);
		float extra_radius = range / 20;
		Box2d box(pos);
		box.v1.x -= obj->size.x;
		box.v1.y -= obj->size.y;
		box.v2.x += obj->size.x;
		box.v2.y += obj->size.y;
		for(int i = 0; i < 20; ++i)
		{
			if(!Collide(global_col, box, margin, rot))
			{
				ok = true;
				break;
			}
			pt = Vec3(pos.x, 0, pos.y);
			pt += Vec2::RandomPoissonDiscPoint().XZ() * extra_radius;
			extra_radius += range / 20;
			box.v1.x = pt.x - obj->size.x;
			box.v1.y = pt.z - obj->size.y;
			box.v2.x = pt.x + obj->size.x;
			box.v2.y = pt.z + obj->size.y;
		}

		if(!ok)
			return nullptr;

		if(area.area_type == LevelArea::Type::Outside)
			terrain->SetY(pt);

		return SpawnObjectEntity(area, obj, pt, rot, scale);
	}
}

//=================================================================================================
ObjectEntity Level::SpawnObjectNearLocation(LevelArea& area, BaseObject* obj, const Vec2& pos, const Vec2& rot_target, float range, float margin,
	float scale)
{
	if(obj->type == OBJ_CYLINDER)
		return SpawnObjectNearLocation(area, obj, pos, Vec2::LookAtAngle(pos, rot_target), range, margin, scale);
	else
	{
		global_col.clear();
		Vec3 pt(pos.x, 0, pos.y);
		GatherCollisionObjects(area, global_col, pt, sqrt(obj->size.x + obj->size.y) + margin + range);
		float extra_radius = range / 20, rot;
		Box2d box(pos);
		box.v1.x -= obj->size.x;
		box.v1.y -= obj->size.y;
		box.v2.x += obj->size.x;
		box.v2.y += obj->size.y;
		bool ok = false;
		for(int i = 0; i < 20; ++i)
		{
			rot = Vec2::LookAtAngle(Vec2(pt.x, pt.z), rot_target);
			if(!Collide(global_col, box, margin, rot))
			{
				ok = true;
				break;
			}
			pt = Vec3(pos.x, 0, pos.y);
			pt += Vec2::RandomPoissonDiscPoint().XZ() * extra_radius;
			extra_radius += range / 20;
			box.v1.x = pt.x - obj->size.x;
			box.v1.y = pt.z - obj->size.y;
			box.v2.x = pt.x + obj->size.x;
			box.v2.y = pt.z + obj->size.y;
		}

		if(!ok)
			return nullptr;

		if(area.area_type == LevelArea::Type::Outside)
			terrain->SetY(pt);

		return SpawnObjectEntity(area, obj, pt, rot, scale);
	}
}

//=================================================================================================
void Level::PickableItemBegin(LevelArea& area, Object& o)
{
	assert(o.base);

	pickable_area = &area;
	pickable_obj = &o;
	pickable_spawns.clear();
	pickable_items.clear();

	for(vector<Mesh::Point>::iterator it = o.base->mesh->attach_points.begin(), end = o.base->mesh->attach_points.end(); it != end; ++it)
	{
		if(strncmp(it->name.c_str(), "spawn_", 6) == 0)
		{
			assert(it->type == Mesh::Point::BOX);
			Box box(it->mat._41, it->mat._42, it->mat._43);
			box.v1.x -= it->size.x - 0.05f;
			box.v1.z -= it->size.z - 0.05f;
			box.v2.x += it->size.x - 0.05f;
			box.v2.z += it->size.z - 0.05f;
			box.v1.y = box.v2.y = box.v1.y - it->size.y;
			pickable_spawns.push_back(box);
		}
	}

	assert(!pickable_spawns.empty());
}

//=================================================================================================
bool Level::PickableItemAdd(const Item* item)
{
	assert(item);

	for(int i = 0; i < 20; ++i)
	{
		// pobierz punkt
		uint spawn = Rand() % pickable_spawns.size();
		Box& box = pickable_spawns[spawn];
		// ustal pozycj�
		Vec3 pos(Random(box.v1.x, box.v2.x), box.v1.y, Random(box.v1.z, box.v2.z));
		// sprawd� kolizj�
		bool ok = true;
		for(vector<PickableItem>::iterator it = pickable_items.begin(), end = pickable_items.end(); it != end; ++it)
		{
			if(it->spawn == spawn)
			{
				if(CircleToCircle(pos.x, pos.z, 0.15f, it->pos.x, it->pos.z, 0.15f))
				{
					ok = false;
					break;
				}
			}
		}
		// dodaj
		if(ok)
		{
			PickableItem& i = Add1(pickable_items);
			i.spawn = spawn;
			i.pos = pos;

			GroundItem* gi = new GroundItem;
			gi->Register();
			gi->count = 1;
			gi->team_count = 1;
			gi->item = item;
			gi->rot = Quat::RotY(Random(MAX_ANGLE));
			float rot = pickable_obj->rot.y,
				s = sin(rot),
				c = cos(rot);
			gi->pos = Vec3(pos.x * c + pos.z * s, pos.y, -pos.x * s + pos.z * c) + pickable_obj->pos;
			pickable_area->items.push_back(gi);

			return true;
		}
	}

	return false;
}

//=================================================================================================
void Level::PickableItemsFromStock(LevelArea& area, Object& o, Stock& stock)
{
	pickable_tmp_stock.clear();
	stock.Parse(pickable_tmp_stock);

	if(!pickable_tmp_stock.empty())
	{
		PickableItemBegin(area, o);
		for(ItemSlot& slot : pickable_tmp_stock)
		{
			for(uint i = 0; i < slot.count; ++i)
			{
				if(!PickableItemAdd(slot.item))
					return;
			}
		}
	}
}

//=================================================================================================
void Level::AddGroundItem(LevelArea& area, GroundItem* item)
{
	assert(item);

	if(area.area_type == LevelArea::Type::Outside)
		terrain->SetY(item->pos);
	area.items.push_back(item);

	if(Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::SPAWN_ITEM;
		c.item = item;
	}
}

//=================================================================================================
GroundItem* Level::FindGroundItem(int id, LevelArea** out_area)
{
	for(LevelArea& area : ForEachArea())
	{
		for(GroundItem* ground_item : area.items)
		{
			if(ground_item->id == id)
			{
				if(out_area)
					*out_area = &area;
				return ground_item;
			}
		}
	}

	return nullptr;
}

//=================================================================================================
GroundItem* Level::SpawnGroundItemInsideAnyRoom(const Item* item)
{
	assert(lvl && item);
	while(true)
	{
		int id = Rand() % lvl->rooms.size();
		if(!lvl->rooms[id]->IsCorridor())
		{
			GroundItem* item2 = SpawnGroundItemInsideRoom(*lvl->rooms[id], item);
			if(item2)
				return item2;
		}
	}
}

//=================================================================================================
GroundItem* Level::SpawnGroundItemInsideRoom(Room& room, const Item* item)
{
	assert(item);

	for(int i = 0; i < 50; ++i)
	{
		Vec3 pos = room.GetRandomPos(0.5f);
		global_col.clear();
		GatherCollisionObjects(*local_area, global_col, pos, 0.25f);
		if(!Collide(global_col, pos, 0.25f))
			return SpawnItem(item, pos);
	}

	return nullptr;
}

//=================================================================================================
GroundItem* Level::SpawnGroundItemInsideRadius(const Item* item, const Vec2& pos, float radius, bool try_exact)
{
	assert(item);

	Vec3 pt;
	for(int i = 0; i < 50; ++i)
	{
		if(!try_exact)
		{
			float a = Random(), b = Random();
			if(b < a)
				std::swap(a, b);
			pt = Vec3(pos.x + b * radius * cos(2 * PI * a / b), 0, pos.y + b * radius * sin(2 * PI * a / b));
		}
		else
		{
			try_exact = false;
			pt = Vec3(pos.x, 0, pos.y);
		}
		global_col.clear();
		GatherCollisionObjects(*local_area, global_col, pt, 0.25f);
		if(!Collide(global_col, pt, 0.25f))
			return SpawnItem(item, pt);
	}

	return nullptr;
}

//=================================================================================================
GroundItem* Level::SpawnGroundItemInsideRegion(const Item* item, const Vec2& pos, const Vec2& region_size, bool try_exact)
{
	assert(item);
	assert(region_size.x > 0 && region_size.y > 0);

	Vec3 pt;
	for(int i = 0; i < 50; ++i)
	{
		if(!try_exact)
			pt = Vec3(pos.x + Random(region_size.x), 0, pos.y + Random(region_size.y));
		else
		{
			try_exact = false;
			pt = Vec3(pos.x, 0, pos.y);
		}
		global_col.clear();
		GatherCollisionObjects(*local_area, global_col, pt, 0.25f);
		if(!Collide(global_col, pt, 0.25f))
			return SpawnItem(item, pt);
	}

	return nullptr;
}

//=================================================================================================
Unit* Level::CreateUnit(UnitData& base, int level, bool create_physics)
{
	Unit* unit = new Unit;
	unit->Init(base, level);

	// preload items
	if(base.group != G_PLAYER && base.item_script && !res_mgr->IsLoadScreen())
	{
		for(const Item* slot : unit->slots)
		{
			if(slot)
				game_res->PreloadItem(slot);
		}
		for(ItemSlot& slot : unit->items)
			game_res->PreloadItem(slot.item);
	}
	if(base.trader && !entering)
	{
		for(ItemSlot& slot : unit->stock->items)
			game_res->PreloadItem(slot.item);
	}

	// mesh
	unit->CreateMesh(Unit::CREATE_MESH::NORMAL);

	// physics
	if(create_physics)
		unit->CreatePhysics();
	else
		unit->cobj = nullptr;

	if(Net::IsServer() && !entering)
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::SPAWN_UNIT;
		c.unit = unit;
	}

	return unit;
}

//=================================================================================================
Unit* Level::CreateUnitWithAI(LevelArea& area, UnitData& unit, int level, const Vec3* pos, const float* rot)
{
	Unit* u = CreateUnit(unit, level);
	u->area = &area;
	area.units.push_back(u);

	if(pos)
	{
		if(area.area_type == LevelArea::Type::Outside)
		{
			Vec3 pt = *pos;
			game_level->terrain->SetY(pt);
			u->pos = pt;
		}
		else
			u->pos = *pos;
		u->UpdatePhysics();
		u->visual_pos = u->pos;
	}

	if(rot)
		u->rot = *rot;

	AIController* ai = new AIController;
	ai->Init(u);
	game->ais.push_back(ai);

	return u;
}

//=================================================================================================
Vec3 Level::FindSpawnPos(Room* room, Unit* unit)
{
	assert(room && unit);

	const float radius = unit->data->GetRadius();

	for(int i = 0; i < 10; ++i)
	{
		Vec3 pos = room->GetRandomPos(radius);
		Int2 pt = PosToPt(pos);

		Tile& tile = lvl->map[pt(lvl->w)];
		if(Any(tile.type, ENTRY_PREV, ENTRY_NEXT))
			continue;

		global_col.clear();
		GatherCollisionObjects(*local_area, global_col, pos, radius, nullptr);
		if(!Collide(global_col, pos, radius))
			return pos;
	}

	return room->Center();
}

//=================================================================================================
Unit* Level::SpawnUnitInsideRoom(Room& room, UnitData& unit, int level, const Int2& awayPt, const Int2& excludedPt)
{
	const float radius = unit.GetRadius();
	const Vec3 awayPos(2.f * awayPt.x + 1.f, 0.f, 2.f * awayPt.y + 1.f);

	for(int i = 0; i < 10; ++i)
	{
		const Vec3 pos = room.GetRandomPos(radius);
		if(Vec3::Distance(awayPos, pos) < ALERT_SPAWN_RANGE)
			continue;

		const Int2 pt = Int2(int(pos.x / 2), int(pos.y / 2));
		if(pt == excludedPt)
			continue;

		global_col.clear();
		GatherCollisionObjects(*local_area, global_col, pos, radius, nullptr);

		if(!Collide(global_col, pos, radius))
		{
			float rot = Random(MAX_ANGLE);
			return CreateUnitWithAI(*local_area, unit, level, &pos, &rot);
		}
	}

	return nullptr;
}

//=================================================================================================
Unit* Level::SpawnUnitInsideRoomOrNear(Room& room, UnitData& ud, int level, const Int2& awayPt, const Int2& excludedPt)
{
	Unit* u = SpawnUnitInsideRoom(room, ud, level, awayPt, excludedPt);
	if(u)
		return u;

	LocalVector<Room*> connected(room.connected);
	connected.Shuffle();

	for(Room* room : connected)
	{
		u = SpawnUnitInsideRoom(*room, ud, level, awayPt, excludedPt);
		if(u)
			return u;
	}

	return nullptr;
}

//=================================================================================================
Unit* Level::SpawnUnitNearLocation(LevelArea& area, const Vec3& pos, UnitData& unit, const Vec3* look_at, int level, float extra_radius)
{
	const float radius = unit.GetRadius();

	global_col.clear();
	GatherCollisionObjects(area, global_col, pos, extra_radius + radius, nullptr);

	Vec3 tmp_pos = pos;

	for(int i = 0; i < 10; ++i)
	{
		if(!Collide(global_col, tmp_pos, radius))
		{
			float rot;
			if(look_at)
				rot = Vec3::LookAtAngle(tmp_pos, *look_at);
			else
				rot = Random(MAX_ANGLE);
			return CreateUnitWithAI(area, unit, level, &tmp_pos, &rot);
		}

		tmp_pos = pos + Vec2::RandomPoissonDiscPoint().XZ() * extra_radius;
	}

	return nullptr;
}

//=================================================================================================
Unit* Level::SpawnUnitInsideRegion(LevelArea& area, const Box2d& region, UnitData& unit, int level)
{
	Vec3 pos;
	if(!WarpToRegion(area, region, unit.GetRadius(), pos))
		return nullptr;

	return CreateUnitWithAI(area, unit, level, &pos);
}

//=================================================================================================
Unit* Level::SpawnUnitInsideInn(UnitData& ud, int level, InsideBuilding* inn, int flags)
{
	if(!inn)
		inn = city_ctx->FindInn();

	Vec3 pos;
	bool ok = false;
	if(IsSet(flags, SU_MAIN_ROOM) || Rand() % 5 != 0)
	{
		if(WarpToRegion(*inn, inn->region1, ud.GetRadius(), pos, 20)
			|| WarpToRegion(*inn, inn->region2, ud.GetRadius(), pos, 10))
			ok = true;
	}
	else
	{
		if(WarpToRegion(*inn, inn->region2, ud.GetRadius(), pos, 10)
			|| WarpToRegion(*inn, inn->region1, ud.GetRadius(), pos, 20))
			ok = true;
	}

	if(ok)
	{
		float rot = Random(MAX_ANGLE);
		Unit* u = CreateUnitWithAI(*inn, ud, level, &pos, &rot);
		if(u && IsSet(flags, SU_TEMPORARY))
			u->temporary = true;
		return u;
	}
	else
		return nullptr;
}

//=================================================================================================
void Level::SpawnUnitsGroup(LevelArea& area, const Vec3& pos, const Vec3* look_at, uint count, UnitGroup* group, int level, delegate<void(Unit*)> callback)
{
	Pooled<TmpUnitGroup> tgroup;
	tgroup->Fill(group, level);

	for(TmpUnitGroup::Spawn& spawn : tgroup->Roll(level, count))
	{
		Unit* u = SpawnUnitNearLocation(area, pos, *spawn.first, look_at, spawn.second, 4.f);
		if(u && callback)
			callback(u);
	}
}

//=================================================================================================
Unit* Level::SpawnUnit(LevelArea& area, TmpSpawn spawn)
{
	assert(area.area_type == LevelArea::Type::Building); // not implemented

	InsideBuilding& building = static_cast<InsideBuilding&>(area);
	Vec3 pos;
	if(!WarpToRegion(area, building.region1, spawn.first->GetRadius(), pos))
		return nullptr;

	float rot = Random(MAX_ANGLE);
	return CreateUnitWithAI(area, *spawn.first, spawn.second, &pos, &rot);
}

//=================================================================================================
void Level::GatherCollisionObjects(LevelArea& area, vector<CollisionObject>& objects, const Vec3& _pos, float _radius, const IgnoreObjects* ignore)
{
	assert(_radius > 0.f);

	// tiles
	int minx = max(0, int((_pos.x - _radius) / 2)),
		maxx = int((_pos.x + _radius) / 2),
		minz = max(0, int((_pos.z - _radius) / 2)),
		maxz = int((_pos.z + _radius) / 2);

	if((!ignore || !ignore->ignore_blocks) && area.area_type != LevelArea::Type::Building)
	{
		if(location->outside)
		{
			City* city = static_cast<City*>(location);
			TerrainTile* tiles = city->tiles;
			maxx = min(maxx, OutsideLocation::size);
			maxz = min(maxz, OutsideLocation::size);

			for(int z = minz; z <= maxz; ++z)
			{
				for(int x = minx; x <= maxx; ++x)
				{
					if(tiles[x + z * OutsideLocation::size].IsBlocking())
					{
						CollisionObject& co = Add1(objects);
						co.pos = Vec3(2.f * x + 1.f, 0, 2.f * z + 1.f);
						co.w = 1.f;
						co.h = 1.f;
						co.type = CollisionObject::RECTANGLE;
					}
				}
			}
		}
		else
		{
			maxx = min(maxx, lvl->w);
			maxz = min(maxz, lvl->h);

			for(int z = minz; z <= maxz; ++z)
			{
				for(int x = minx; x <= maxx; ++x)
				{
					TILE_TYPE type = lvl->map[x + z * lvl->w].type;
					if(IsBlocking(type))
					{
						CollisionObject& co = Add1(objects);
						co.pos = Vec3(2.f * x + 1.f, 0, 2.f * z + 1.f);
						co.w = 1.f;
						co.h = 1.f;
						co.type = CollisionObject::RECTANGLE;
					}
					else if(type == ENTRY_NEXT || type == ENTRY_PREV)
					{
						EntryType entryType = type == ENTRY_NEXT ? lvl->nextEntryType : lvl->prevEntryType;
						if(entryType == ENTRY_STAIRS_UP || entryType == ENTRY_STAIRS_DOWN)
						{
							CollisionObject& co = Add1(objects);
							co.pos = Vec3(2.f * x + 1.f, 0, 2.f * z + 1.f);
							co.check = &Level::CollideWithStairs;
							co.check_rect = &Level::CollideWithStairsRect;
							co.extra = (type == ENTRY_PREV);
							co.type = CollisionObject::CUSTOM;
						}
					}
				}
			}
		}
	}

	// units
	if(!(ignore && ignore->ignore_units))
	{
		float radius;
		Vec3 pos;
		if(ignore && ignore->ignored_units)
		{
			for(vector<Unit*>::iterator it = area.units.begin(), end = area.units.end(); it != end; ++it)
			{
				if(!*it || !(*it)->IsStanding())
					continue;

				const Unit** u = ignore->ignored_units;
				do
				{
					if(!*u)
						break;
					if(*u == *it)
						goto ignore_unit;
					++u;
				}
				while(1);

				radius = (*it)->GetUnitRadius();
				pos = (*it)->GetColliderPos();
				if(Distance(pos.x, pos.z, _pos.x, _pos.z) <= radius + _radius)
				{
					CollisionObject& co = Add1(objects);
					co.pos = pos;
					co.radius = radius;
					co.type = CollisionObject::SPHERE;
				}

			ignore_unit:
				;
			}
		}
		else
		{
			for(vector<Unit*>::iterator it = area.units.begin(), end = area.units.end(); it != end; ++it)
			{
				if(!*it || !(*it)->IsStanding())
					continue;

				radius = (*it)->GetUnitRadius();
				Vec3 pos = (*it)->GetColliderPos();
				if(Distance(pos.x, pos.z, _pos.x, _pos.z) <= radius + _radius)
				{
					CollisionObject& co = Add1(objects);
					co.pos = pos;
					co.radius = radius;
					co.type = CollisionObject::SPHERE;
				}
			}
		}
	}

	// object colliders
	if(!(ignore && ignore->ignore_objects))
	{
		if(!ignore || !ignore->ignored_objects)
		{
			for(vector<CollisionObject>::iterator it = area.tmp->colliders.begin(), end = area.tmp->colliders.end(); it != end; ++it)
			{
				if(it->type == CollisionObject::RECTANGLE)
				{
					if(CircleToRectangle(_pos.x, _pos.z, _radius, it->pos.x, it->pos.z, it->w, it->h))
						objects.push_back(*it);
				}
				else
				{
					if(CircleToCircle(_pos.x, _pos.z, _radius, it->pos.x, it->pos.z, it->radius))
						objects.push_back(*it);
				}
			}
		}
		else
		{
			for(vector<CollisionObject>::iterator it = area.tmp->colliders.begin(), end = area.tmp->colliders.end(); it != end; ++it)
			{
				if(it->owner)
				{
					const void** objs = ignore->ignored_objects;
					do
					{
						if(it->owner == *objs)
							goto ignore;
						else if(!*objs)
							break;
						else
							++objs;
					}
					while(1);
				}

				if(it->type == CollisionObject::RECTANGLE)
				{
					if(CircleToRectangle(_pos.x, _pos.z, _radius, it->pos.x, it->pos.z, it->w, it->h))
						objects.push_back(*it);
				}
				else
				{
					if(CircleToCircle(_pos.x, _pos.z, _radius, it->pos.x, it->pos.z, it->radius))
						objects.push_back(*it);
				}

			ignore:
				;
			}
		}
	}

	// doors
	if(!(ignore && ignore->ignore_doors))
	{
		for(vector<Door*>::iterator it = area.doors.begin(), end = area.doors.end(); it != end; ++it)
		{
			if((*it)->IsBlocking() && CircleToRotatedRectangle(_pos.x, _pos.z, _radius, (*it)->pos.x, (*it)->pos.z, Door::WIDTH, Door::THICKNESS, (*it)->rot))
			{
				CollisionObject& co = Add1(objects);
				co.pos = (*it)->pos;
				co.type = CollisionObject::RECTANGLE_ROT;
				co.w = Door::WIDTH;
				co.h = Door::THICKNESS;
				co.rot = (*it)->rot;
			}
		}
	}
}

//=================================================================================================
void Level::GatherCollisionObjects(LevelArea& area, vector<CollisionObject>& objects, const Box2d& _box, const IgnoreObjects* ignore)
{
	// tiles
	int minx = max(0, int(_box.v1.x / 2)),
		maxx = int(_box.v2.x / 2),
		minz = max(0, int(_box.v1.y / 2)),
		maxz = int(_box.v2.y / 2);

	if((!ignore || !ignore->ignore_blocks) && area.area_type != LevelArea::Type::Building)
	{
		if(location->outside)
		{
			City* city = static_cast<City*>(location);
			TerrainTile* tiles = city->tiles;
			maxx = min(maxx, OutsideLocation::size);
			maxz = min(maxz, OutsideLocation::size);

			for(int z = minz; z <= maxz; ++z)
			{
				for(int x = minx; x <= maxx; ++x)
				{
					if(tiles[x + z * OutsideLocation::size].IsBlocking())
					{
						CollisionObject& co = Add1(objects);
						co.pos = Vec3(2.f * x + 1.f, 0, 2.f * z + 1.f);
						co.w = 1.f;
						co.h = 1.f;
						co.type = CollisionObject::RECTANGLE;
					}
				}
			}
		}
		else
		{
			maxx = min(maxx, lvl->w);
			maxz = min(maxz, lvl->h);

			for(int z = minz; z <= maxz; ++z)
			{
				for(int x = minx; x <= maxx; ++x)
				{
					TILE_TYPE type = lvl->map[x + z * lvl->w].type;
					if(IsBlocking(type))
					{
						CollisionObject& co = Add1(objects);
						co.pos = Vec3(2.f * x + 1.f, 0, 2.f * z + 1.f);
						co.w = 1.f;
						co.h = 1.f;
						co.type = CollisionObject::RECTANGLE;
					}
					else if(type == ENTRY_NEXT || type == ENTRY_PREV)
					{
						EntryType entryType = type == ENTRY_NEXT ? lvl->nextEntryType : lvl->prevEntryType;
						if(entryType == ENTRY_STAIRS_UP || entryType == ENTRY_STAIRS_DOWN)
						{
							CollisionObject& co = Add1(objects);
							co.pos = Vec3(2.f * x + 1.f, 0, 2.f * z + 1.f);
							co.check = &Level::CollideWithStairs;
							co.check_rect = &Level::CollideWithStairsRect;
							co.extra = (type == ENTRY_PREV);
							co.type = CollisionObject::CUSTOM;
						}
					}
				}
			}
		}
	}

	Vec2 rectpos = _box.Midpoint(),
		rectsize = _box.Size();

	// units
	if(!(ignore && ignore->ignore_units))
	{
		float radius;
		if(ignore && ignore->ignored_units)
		{
			for(vector<Unit*>::iterator it = area.units.begin(), end = area.units.end(); it != end; ++it)
			{
				if(!(*it)->IsStanding())
					continue;

				const Unit** u = ignore->ignored_units;
				do
				{
					if(!*u)
						break;
					if(*u == *it)
						goto ignore_unit;
					++u;
				}
				while(1);

				radius = (*it)->GetUnitRadius();
				if(CircleToRectangle((*it)->pos.x, (*it)->pos.z, radius, rectpos.x, rectpos.y, rectsize.x, rectsize.y))
				{
					CollisionObject& co = Add1(objects);
					co.pos = (*it)->pos;
					co.radius = radius;
					co.type = CollisionObject::SPHERE;
				}

			ignore_unit:
				;
			}
		}
		else
		{
			for(vector<Unit*>::iterator it = area.units.begin(), end = area.units.end(); it != end; ++it)
			{
				if(!(*it)->IsStanding())
					continue;

				radius = (*it)->GetUnitRadius();
				if(CircleToRectangle((*it)->pos.x, (*it)->pos.z, radius, rectpos.x, rectpos.y, rectsize.x, rectsize.y))
				{
					CollisionObject& co = Add1(objects);
					co.pos = (*it)->pos;
					co.radius = radius;
					co.type = CollisionObject::SPHERE;
				}
			}
		}
	}

	// object colliders
	if(!(ignore && ignore->ignore_objects))
	{
		if(!ignore || !ignore->ignored_objects)
		{
			for(vector<CollisionObject>::iterator it = area.tmp->colliders.begin(), end = area.tmp->colliders.end(); it != end; ++it)
			{
				if(it->type == CollisionObject::RECTANGLE)
				{
					if(RectangleToRectangle(it->pos.x - it->w, it->pos.z - it->h, it->pos.x + it->w, it->pos.z + it->h, _box.v1.x, _box.v1.y, _box.v2.x, _box.v2.y))
						objects.push_back(*it);
				}
				else
				{
					if(CircleToRectangle(it->pos.x, it->pos.z, it->radius, rectpos.x, rectpos.y, rectsize.x, rectsize.y))
						objects.push_back(*it);
				}
			}
		}
		else
		{
			for(vector<CollisionObject>::iterator it = area.tmp->colliders.begin(), end = area.tmp->colliders.end(); it != end; ++it)
			{
				if(it->owner)
				{
					const void** objs = ignore->ignored_objects;
					do
					{
						if(it->owner == *objs)
							goto ignore;
						else if(!*objs)
							break;
						else
							++objs;
					}
					while(1);
				}

				if(it->type == CollisionObject::RECTANGLE)
				{
					if(RectangleToRectangle(it->pos.x - it->w, it->pos.z - it->h, it->pos.x + it->w, it->pos.z + it->h, _box.v1.x, _box.v1.y, _box.v2.x, _box.v2.y))
						objects.push_back(*it);
				}
				else
				{
					if(CircleToRectangle(it->pos.x, it->pos.z, it->radius, rectpos.x, rectpos.y, rectsize.x, rectsize.y))
						objects.push_back(*it);
				}

			ignore:
				;
			}
		}
	}
}

//=================================================================================================
bool Level::Collide(const vector<CollisionObject>& objects, const Vec3& pos, float radius)
{
	assert(radius > 0.f);

	for(vector<CollisionObject>::const_iterator it = objects.begin(), end = objects.end(); it != end; ++it)
	{
		switch(it->type)
		{
		case CollisionObject::SPHERE:
			if(Distance(pos.x, pos.z, it->pos.x, it->pos.z) <= it->radius + radius)
				return true;
			break;
		case CollisionObject::RECTANGLE:
			if(CircleToRectangle(pos.x, pos.z, radius, it->pos.x, it->pos.z, it->w, it->h))
				return true;
			break;
		case CollisionObject::RECTANGLE_ROT:
			if(CircleToRotatedRectangle(pos.x, pos.z, radius, it->pos.x, it->pos.z, it->w, it->h, it->rot))
				return true;
			break;
		case CollisionObject::CUSTOM:
			if((this->*(it->check))(*it, pos, radius))
				return true;
			break;
		}
	}

	return false;
}

//=================================================================================================
bool Level::Collide(const vector<CollisionObject>& objects, const Box2d& _box, float _margin)
{
	Box2d box = _box;
	box.v1.x -= _margin;
	box.v1.y -= _margin;
	box.v2.x += _margin;
	box.v2.y += _margin;

	Vec2 rectpos = _box.Midpoint(),
		rectsize = _box.Size() / 2;

	for(vector<CollisionObject>::const_iterator it = objects.begin(), end = objects.end(); it != end; ++it)
	{
		switch(it->type)
		{
		case CollisionObject::SPHERE:
			if(CircleToRectangle(it->pos.x, it->pos.z, it->radius + _margin, rectpos.x, rectpos.y, rectsize.x, rectsize.y))
				return true;
			break;
		case CollisionObject::RECTANGLE:
			if(RectangleToRectangle(box.v1.x, box.v1.y, box.v2.x, box.v2.y, it->pos.x - it->w, it->pos.z - it->h, it->pos.x + it->w, it->pos.z + it->h))
				return true;
			break;
		case CollisionObject::RECTANGLE_ROT:
			{
				RotRect r1, r2;
				r1.center = it->pos.XZ();
				r1.size.x = it->w + _margin;
				r1.size.y = it->h + _margin;
				r1.rot = -it->rot;
				r2.center = rectpos;
				r2.size = rectsize;
				r2.rot = 0.f;

				if(RotatedRectanglesCollision(r1, r2))
					return true;
			}
			break;
		case CollisionObject::CUSTOM:
			if((this->*(it->check_rect))(*it, box))
				return true;
			break;
		}
	}

	return false;
}

//=================================================================================================
bool Level::Collide(const vector<CollisionObject>& objects, const Box2d& _box, float margin, float _rot)
{
	if(!NotZero(_rot))
		return Collide(objects, _box, margin);

	Box2d box = _box;
	box.v1.x -= margin;
	box.v1.y -= margin;
	box.v2.x += margin;
	box.v2.y += margin;

	Vec2 rectpos = box.Midpoint(),
		rectsize = box.Size() / 2;

	for(vector<CollisionObject>::const_iterator it = objects.begin(), end = objects.end(); it != end; ++it)
	{
		switch(it->type)
		{
		case CollisionObject::SPHERE:
			if(CircleToRotatedRectangle(it->pos.x, it->pos.z, it->radius, rectpos.x, rectpos.y, rectsize.x, rectsize.y, _rot))
				return true;
			break;
		case CollisionObject::RECTANGLE:
			{
				RotRect r1, r2;
				r1.center = it->pos.XZ();
				r1.size.x = it->w;
				r1.size.y = it->h;
				r1.rot = 0.f;
				r2.center = rectpos;
				r2.size = rectsize;
				r2.rot = _rot;

				if(RotatedRectanglesCollision(r1, r2))
					return true;
			}
			break;
		case CollisionObject::RECTANGLE_ROT:
			{
				RotRect r1, r2;
				r1.center = it->pos.XZ();
				r1.size.x = it->w;
				r1.size.y = it->h;
				r1.rot = it->rot;
				r2.center = rectpos;
				r2.size = rectsize;
				r2.rot = _rot;

				if(RotatedRectanglesCollision(r1, r2))
					return true;
			}
			break;
		case CollisionObject::CUSTOM:
			if((this->*(it->check_rect))(*it, box))
				return true;
			break;
		}
	}

	return false;
}

//=================================================================================================
bool Level::CollideWithStairs(const CollisionObject& cobj, const Vec3& pos, float radius) const
{
	assert(cobj.type == CollisionObject::CUSTOM && cobj.check == &Level::CollideWithStairs && !location->outside && radius > 0.f);

	GameDirection dir;
	if(cobj.extra == 0)
		dir = lvl->nextEntryDir;
	else
		dir = lvl->prevEntryDir;

	if(dir != GDIR_DOWN)
	{
		if(CircleToRectangle(pos.x, pos.z, radius, cobj.pos.x, cobj.pos.z - 0.95f, 1.f, 0.05f))
			return true;
	}

	if(dir != GDIR_LEFT)
	{
		if(CircleToRectangle(pos.x, pos.z, radius, cobj.pos.x - 0.95f, cobj.pos.z, 0.05f, 1.f))
			return true;
	}

	if(dir != GDIR_UP)
	{
		if(CircleToRectangle(pos.x, pos.z, radius, cobj.pos.x, cobj.pos.z + 0.95f, 1.f, 0.05f))
			return true;
	}

	if(dir != GDIR_RIGHT)
	{
		if(CircleToRectangle(pos.x, pos.z, radius, cobj.pos.x + 0.95f, cobj.pos.z, 0.05f, 1.f))
			return true;
	}

	return false;
}

//=================================================================================================
bool Level::CollideWithStairsRect(const CollisionObject& cobj, const Box2d& box) const
{
	assert(cobj.type == CollisionObject::CUSTOM && cobj.check_rect == &Level::CollideWithStairsRect && !location->outside);

	GameDirection dir;
	if(cobj.extra == 0)
		dir = lvl->nextEntryDir;
	else
		dir = lvl->prevEntryDir;

	if(dir != GDIR_DOWN)
	{
		if(RectangleToRectangle(box.v1.x, box.v1.y, box.v2.x, box.v2.y, cobj.pos.x - 1.f, cobj.pos.z - 1.f, cobj.pos.x + 1.f, cobj.pos.z - 0.9f))
			return true;
	}

	if(dir != GDIR_LEFT)
	{
		if(RectangleToRectangle(box.v1.x, box.v1.y, box.v2.x, box.v2.y, cobj.pos.x - 1.f, cobj.pos.z - 1.f, cobj.pos.x - 0.9f, cobj.pos.z + 1.f))
			return true;
	}

	if(dir != GDIR_UP)
	{
		if(RectangleToRectangle(box.v1.x, box.v1.y, box.v2.x, box.v2.y, cobj.pos.x - 1.f, cobj.pos.z + 0.9f, cobj.pos.x + 1.f, cobj.pos.z + 1.f))
			return true;
	}

	if(dir != GDIR_RIGHT)
	{
		if(RectangleToRectangle(box.v1.x, box.v1.y, box.v2.x, box.v2.y, cobj.pos.x + 0.9f, cobj.pos.z - 1.f, cobj.pos.x + 1.f, cobj.pos.z + 1.f))
			return true;
	}

	return false;
}

//=================================================================================================
void Level::CreateBlood(LevelArea& area, const Unit& u, bool fully_created)
{
	if(!game_res->tBloodSplat[u.data->blood] || IsSet(u.data->flags2, F2_BLOODLESS))
		return;

	Blood* blood = new Blood;
	u.mesh_inst->SetupBones();
	blood->pos = u.GetLootCenter();
	blood->type = u.data->blood;
	blood->rot = Random(MAX_ANGLE);
	blood->size = (fully_created ? 1.f : 0.f);
	blood->scale = u.data->blood_size;

	if(area.have_terrain)
	{
		blood->pos.y = terrain->GetH(blood->pos) + 0.05f;
		terrain->GetAngle(blood->pos.x, blood->pos.z, blood->normal);
	}
	else
	{
		blood->pos.y = u.pos.y + 0.05f;
		blood->normal = Vec3(0, 1, 0);
	}

	area.bloods.push_back(blood);
}

//=================================================================================================
void Level::SpawnBlood()
{
	for(Unit* unit : blood_to_spawn)
		CreateBlood(*unit->area, *unit, true);
	blood_to_spawn.clear();
}

//=================================================================================================
void Level::WarpUnit(Unit& unit, const Vec3& pos)
{
	const float unit_radius = unit.GetUnitRadius();

	unit.BreakAction(Unit::BREAK_ACTION_MODE::INSTANT, false, true);

	global_col.clear();
	LevelArea& area = *unit.area;
	Level::IgnoreObjects ignore = { 0 };
	const Unit* ignore_units[2] = { &unit, nullptr };
	ignore.ignored_units = ignore_units;
	GatherCollisionObjects(area, global_col, pos, 2.f + unit_radius, &ignore);

	Vec3 tmp_pos = pos;
	bool ok = false;
	float radius = unit_radius;

	for(int i = 0; i < 20; ++i)
	{
		if(!Collide(global_col, tmp_pos, unit_radius))
		{
			unit.pos = tmp_pos;
			ok = true;
			break;
		}

		tmp_pos = pos + Vec2::RandomPoissonDiscPoint().XZ() * radius;

		if(i < 10)
			radius += 0.25f;
	}

	assert(ok);

	if(area.have_terrain && terrain->IsInside(unit.pos))
		terrain->SetY(unit.pos);

	if(unit.cobj)
		unit.UpdatePhysics();

	unit.visual_pos = unit.pos;

	if(Net::IsOnline())
	{
		if(unit.interp)
			unit.interp->Reset(unit.pos, unit.rot);
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::WARP;
		c.unit = &unit;
		if(unit.IsPlayer())
			unit.player->player_info->warping = true;
	}
}

//=================================================================================================
bool Level::WarpToRegion(LevelArea& area, const Box2d& region, float radius, Vec3& pos, int tries)
{
	for(int i = 0; i < tries; ++i)
	{
		pos = region.GetRandomPos3();

		global_col.clear();
		GatherCollisionObjects(area, global_col, pos, radius, nullptr);

		if(!Collide(global_col, pos, radius))
			return true;
	}

	return false;
}

//=================================================================================================
void Level::WarpNearLocation(LevelArea& area, Unit& unit, const Vec3& pos, float extra_radius, bool allow_exact, int tries)
{
	const float radius = unit.GetUnitRadius();

	global_col.clear();
	IgnoreObjects ignore = { 0 };
	const Unit* ignore_units[2] = { &unit, nullptr };
	ignore.ignored_units = ignore_units;
	GatherCollisionObjects(area, global_col, pos, extra_radius + radius, &ignore);

	Vec3 tmp_pos = pos;
	if(!allow_exact)
		tmp_pos += Vec2::RandomPoissonDiscPoint().XZ() * extra_radius;

	for(int i = 0; i < tries; ++i)
	{
		if(!Collide(global_col, tmp_pos, radius))
			break;

		tmp_pos = pos + Vec2::RandomPoissonDiscPoint().XZ() * extra_radius;
	}

	unit.pos = tmp_pos;
	unit.Moved(true);
	unit.visual_pos = unit.pos;

	if(Net::IsOnline())
	{
		if(unit.interp)
			unit.interp->Reset(unit.pos, unit.rot);
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::WARP;
		c.unit = &unit;
		if(unit.IsPlayer())
			unit.player->player_info->warping = true;
	}

	if(unit.cobj)
		unit.UpdatePhysics();
}

//=================================================================================================
Trap* Level::CreateTrap(Int2 pt, TRAP_TYPE type, bool timed)
{
	assert(lvl);

	struct TrapLocation
	{
		Int2 pt;
		int dist;
		GameDirection dir;
	};

	Trap* t = new Trap;
	Trap& trap = *t;
	local_area->traps.push_back(t);

	BaseTrap& base = BaseTrap::traps[type];
	trap.base = &base;
	trap.hitted = nullptr;
	trap.state = 0;
	trap.pos = Vec3(2.f * pt.x + Random(trap.base->rw, 2.f - trap.base->rw), 0.f, 2.f * pt.y + Random(trap.base->h, 2.f - trap.base->h));
	trap.obj.base = nullptr;
	trap.obj.mesh = trap.base->mesh;
	trap.obj.pos = trap.pos;
	trap.obj.scale = 1.f;

	if(type == TRAP_ARROW || type == TRAP_POISON)
	{
		trap.obj.rot = Vec3(0, 0, 0);

		static vector<TrapLocation> possible;

		// ustal tile i dir
		for(int i = 0; i < 4; ++i)
		{
			GameDirection dir = (GameDirection)i;
			bool ok = false;
			int j;

			for(j = 1; j <= 10; ++j)
			{
				if(IsBlocking(lvl->map[pt.x + DirToPos(dir).x * j + (pt.y + DirToPos(dir).y * j) * lvl->w]))
				{
					if(j != 1)
						ok = true;
					break;
				}
			}

			if(ok)
			{
				trap.tile = pt + DirToPos(dir) * j;

				if(CanShootAtLocation(Vec3(trap.pos.x + (2.f * j - 1.2f) * DirToPos(dir).x, 1.f, trap.pos.z + (2.f * j - 1.2f) * DirToPos(dir).y),
					Vec3(trap.pos.x, 1.f, trap.pos.z)))
				{
					TrapLocation& tr = Add1(possible);
					tr.pt = trap.tile;
					tr.dist = j;
					tr.dir = dir;
				}
			}
		}

		if(!possible.empty())
		{
			if(possible.size() > 1)
			{
				std::sort(possible.begin(), possible.end(), [](TrapLocation& pt1, TrapLocation& pt2)
				{
					return abs(pt1.dist - 5) < abs(pt2.dist - 5);
				});
			}

			trap.tile = possible[0].pt;
			trap.dir = possible[0].dir;

			possible.clear();
		}
		else
		{
			local_area->traps.pop_back();
			delete t;
			return nullptr;
		}
	}
	else if(type == TRAP_SPEAR)
	{
		trap.obj.rot = Vec3(0, Random(MAX_ANGLE), 0);
		trap.obj2.base = nullptr;
		trap.obj2.mesh = trap.base->mesh2;
		trap.obj2.pos = trap.obj.pos;
		trap.obj2.rot = trap.obj.rot;
		trap.obj2.scale = 1.f;
		trap.obj2.pos.y -= 2.f;
		trap.hitted = new vector<Unit*>;
	}
	else if(type == TRAP_FIREBALL)
		trap.obj.rot = Vec3(0, PI / 2 * (Rand() % 4), 0);

	if(timed)
	{
		trap.state = -1;
		trap.time = 2.f;
	}

	trap.Register();
	return &trap;
}

//=================================================================================================
void Level::UpdateLocation(int days, int open_chance, bool reset)
{
	for(LevelArea& area : ForEachArea())
	{
		// if reset remove all units
		// otherwise all (>10 days) or some
		if(reset)
		{
			for(Unit* unit : area.units)
			{
				if(unit->IsAlive() && unit->IsHero() && unit->hero->otherTeam)
					unit->hero->otherTeam->Remove();
				delete unit;
			}
			area.units.clear();
		}
		else if(days > 10)
			DeleteElements(area.units, [](Unit* unit) { return !unit->IsAlive(); });
		else
			DeleteElements(area.units, [=](Unit* unit) { return !unit->IsAlive() && Random(4, 10) < days; });

		// remove all ground items (>10 days) or some
		if(days > 10)
			DeleteElements(area.items);
		else
			DeleteElements(area.items, RemoveRandomPred<GroundItem*>(days, 0, 10));

		// heal units
		if(!reset)
		{
			for(Unit* unit : area.units)
			{
				if(unit->IsAlive())
					unit->NaturalHealing(days);
			}
		}

		// remove all blood (>30 days) or some (>5 days)
		if(days > 30)
			DeleteElements(area.bloods);
		else if(days > 5)
			DeleteElements(area.bloods, RemoveRandomPred<Blood*>(days, 4, 30));

		// remove all temporary traps (>30 days) or some (>5 days)
		if(days > 30)
		{
			for(vector<Trap*>::iterator it = area.traps.begin(), end = area.traps.end(); it != end;)
			{
				if((*it)->base->type == TRAP_FIREBALL)
				{
					delete *it;
					if(it + 1 == end)
					{
						area.traps.pop_back();
						break;
					}
					else
					{
						std::iter_swap(it, end - 1);
						area.traps.pop_back();
						end = area.traps.end();
					}
				}
				else
					++it;
			}
		}
		else if(days >= 5)
		{
			for(vector<Trap*>::iterator it = area.traps.begin(), end = area.traps.end(); it != end;)
			{
				if((*it)->base->type == TRAP_FIREBALL && Rand() % 30 < days)
				{
					delete *it;
					if(it + 1 == end)
					{
						area.traps.pop_back();
						break;
					}
					else
					{
						std::iter_swap(it, end - 1);
						area.traps.pop_back();
						end = area.traps.end();
					}
				}
				else
					++it;
			}
		}

		// randomly open/close doors
		for(vector<Door*>::iterator it = area.doors.begin(), end = area.doors.end(); it != end; ++it)
		{
			Door& door = **it;
			if(door.locked == 0)
			{
				if(Rand() % 100 < open_chance)
					door.state = Door::Opened;
				else
					door.state = Door::Closed;
			}
		}
	}
}

//=================================================================================================
int Level::GetDifficultyLevel() const
{
	if(location->outside)
		return location->st;
	else
	{
		InsideLocation* inside = static_cast<InsideLocation*>(location);
		if(inside->IsMultilevel())
		{
			float max_st = (float)inside->st;
			float min_st = max(3.f, max_st * 2 / 3);
			uint levels = static_cast<MultiInsideLocation*>(inside)->levels.size() - 1;
			return (int)Lerp(min_st, max_st, float(dungeon_level) / levels);
		}
		else
			return inside->st;
	}
}

//=================================================================================================
int Level::GetChestDifficultyLevel() const
{
	int st = GetDifficultyLevel();
	if(location->group->IsEmpty())
	{
		if(st > 10)
			return 3;
		else if(st > 5)
			return 2;
		else
			return 1;
	}
	else
		return st;
}

//=================================================================================================
void Level::OnRevisitLevel()
{
	for(LevelArea& area : ForEachArea())
	{
		// recreate doors
		for(Door* door : area.doors)
			door->Recreate();
	}
}

//=================================================================================================
bool Level::HaveArena()
{
	if(city_ctx)
		return IsSet(city_ctx->flags, City::HaveArena);
	return false;
}

//=================================================================================================
InsideBuilding* Level::GetArena()
{
	assert(city_ctx);
	for(InsideBuilding* b : city_ctx->inside_buildings)
	{
		if(b->building->group == BuildingGroup::BG_ARENA)
			return b;
	}
	assert(0);
	return nullptr;
}

//=================================================================================================
cstring Level::GetCurrentLocationText()
{
	if(is_open)
	{
		if(location->outside)
			return location->name.c_str();
		else
		{
			InsideLocation* inside = (InsideLocation*)location;
			if(inside->IsMultilevel())
				return Format(txLocationText, location->name.c_str(), dungeon_level + 1);
			else
				return location->name.c_str();
		}
	}
	else if(location)
		return Format(txLocationTextMap, location->name.c_str());
	else
		return txWorldMap;
}

//=================================================================================================
void Level::CheckIfLocationCleared()
{
	if(city_ctx || location->state == LS_CLEARED)
		return;

	// is current level cleared?
	bool is_clear = true;
	for(vector<Unit*>::iterator it = local_area->units.begin(), end = local_area->units.end(); it != end; ++it)
	{
		if((*it)->IsAlive() && game->pc->unit->IsEnemy(**it, true))
		{
			is_clear = false;
			break;
		}
	}

	// is all level cleared?
	if(is_clear && !location->outside)
	{
		InsideLocation* inside = static_cast<InsideLocation*>(location);
		if(inside->IsMultilevel())
			is_clear = static_cast<MultiInsideLocation*>(inside)->LevelCleared();
	}

	if(is_clear)
	{
		// events v1
		bool prevent = false;
		if(event_handler)
			prevent = event_handler->HandleLocationEvent(LocationEventHandler::CLEARED);

		// events v2
		for(Event& e : location->events)
		{
			if(e.type == EVENT_CLEARED)
			{
				ScriptEvent event(EVENT_CLEARED);
				event.on_cleared.location = location;
				e.quest->FireEvent(event);
			}
		}

		// remove camp in 4-8 days
		if(location->type == L_CAMP)
			static_cast<Camp*>(location)->create_time = world->GetWorldtime() - 30 + Random(4, 8);

		// add news
		if(!prevent && !location->group->IsEmpty())
		{
			if(location->type == L_CAMP)
				world->AddNews(Format(txNewsCampCleared, world->GetNearestSettlement(location->pos)->name.c_str()));
			else
				world->AddNews(Format(txNewsLocCleared, location->name.c_str()));
		}
	}
}

//=================================================================================================
// usuwa podany przedmiot ze �wiata
// u�ywane w que�cie z kamieniem szale�c�w
bool Level::RemoveItemFromWorld(const Item* item)
{
	assert(item);
	for(LevelArea& area : ForEachArea())
	{
		if(area.RemoveItem(item))
			return true;
	}
	return false;
}

//=================================================================================================
void Level::SpawnTerrainCollider()
{
	if(terrain_shape)
		delete terrain_shape;

	terrain_shape = new btHeightfieldTerrainShape(OutsideLocation::size + 1, OutsideLocation::size + 1, terrain->GetHeightMap(), 1.f, 0.f, 10.f, 1, PHY_FLOAT, false);
	terrain_shape->setLocalScaling(btVector3(2.f, 1.f, 2.f));

	obj_terrain = new btCollisionObject;
	obj_terrain->setCollisionShape(terrain_shape);
	obj_terrain->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_TERRAIN);
	obj_terrain->getWorldTransform().setOrigin(btVector3(float(OutsideLocation::size), 5.f, float(OutsideLocation::size)));
	phy_world->addCollisionObject(obj_terrain, CG_TERRAIN);
}

//=================================================================================================
void Level::SpawnDungeonColliders()
{
	assert(lvl);

	InsideLocation* inside = static_cast<InsideLocation*>(location);
	Tile* m = lvl->map;
	int w = lvl->w,
		h = lvl->h;

	for(int y = 1; y < h - 1; ++y)
	{
		for(int x = 1; x < w - 1; ++x)
		{
			if(IsBlocking(m[x + y * w]) && (!IsBlocking(m[x - 1 + (y - 1) * w]) || !IsBlocking(m[x + (y - 1) * w]) || !IsBlocking(m[x + 1 + (y - 1) * w])
				|| !IsBlocking(m[x - 1 + y * w]) || !IsBlocking(m[x + 1 + y * w])
				|| !IsBlocking(m[x - 1 + (y + 1) * w]) || !IsBlocking(m[x + (y + 1) * w]) || !IsBlocking(m[x + 1 + (y + 1) * w])))
			{
				SpawnDungeonCollider(Vec3(2.f * x + 1.f, 2.f, 2.f * y + 1.f));
			}
		}
	}

	// left/right wall
	for(int i = 1; i < h - 1; ++i)
	{
		// left
		if(IsBlocking(m[i * w]) && !IsBlocking(m[1 + i * w]))
			SpawnDungeonCollider(Vec3(1.f, 2.f, 2.f * i + 1.f));

		// right
		if(IsBlocking(m[i * w + w - 1]) && !IsBlocking(m[w - 2 + i * w]))
			SpawnDungeonCollider(Vec3(2.f * (w - 1) + 1.f, 2.f, 2.f * i + 1.f));
	}

	// front/back wall
	for(int i = 1; i < lvl->w - 1; ++i)
	{
		// front
		if(IsBlocking(m[i + (h - 1) * w]) && !IsBlocking(m[i + (h - 2) * w]))
			SpawnDungeonCollider(Vec3(2.f * i + 1.f, 2.f, 2.f * (h - 1) + 1.f));

		// back
		if(IsBlocking(m[i]) && !IsBlocking(m[i + w]))
			SpawnDungeonCollider(Vec3(2.f * i + 1.f, 2.f, 1.f));
	}

	// up stairs
	if(inside->HavePrevEntry())
	{
		btCollisionObject* cobj = new btCollisionObject;
		cobj->setCollisionShape(shape_stairs);
		cobj->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_BUILDING);
		cobj->getWorldTransform().setOrigin(btVector3(2.f * lvl->prevEntryPt.x + 1.f, 0.f, 2.f * lvl->prevEntryPt.y + 1.f));
		cobj->getWorldTransform().setRotation(btQuaternion(DirToRot(lvl->prevEntryDir), 0, 0));
		phy_world->addCollisionObject(cobj, CG_BUILDING);
	}

	// room floors/ceilings
	dungeon_mesh->Clear();
	int index = 0;

	if((inside->type == L_DUNGEON && inside->target == LABYRINTH) || inside->type == L_CAVE)
	{
		const float h = Room::HEIGHT;
		for(int x = 0; x < 16; ++x)
		{
			for(int y = 0; y < 16; ++y)
			{
				// floor
				dungeon_mesh->vertices.push_back(Vec3(2.f * x * lvl->w / 16, 0, 2.f * y * lvl->h / 16));
				dungeon_mesh->vertices.push_back(Vec3(2.f * (x + 1) * lvl->w / 16, 0, 2.f * y * lvl->h / 16));
				dungeon_mesh->vertices.push_back(Vec3(2.f * x * lvl->w / 16, 0, 2.f * (y + 1) * lvl->h / 16));
				dungeon_mesh->vertices.push_back(Vec3(2.f * (x + 1) * lvl->w / 16, 0, 2.f * (y + 1) * lvl->h / 16));
				dungeon_mesh->indices.push_back(index);
				dungeon_mesh->indices.push_back(index + 1);
				dungeon_mesh->indices.push_back(index + 2);
				dungeon_mesh->indices.push_back(index + 2);
				dungeon_mesh->indices.push_back(index + 1);
				dungeon_mesh->indices.push_back(index + 3);
				index += 4;

				// ceil
				dungeon_mesh->vertices.push_back(Vec3(2.f * x * lvl->w / 16, h, 2.f * y * lvl->h / 16));
				dungeon_mesh->vertices.push_back(Vec3(2.f * (x + 1) * lvl->w / 16, h, 2.f * y * lvl->h / 16));
				dungeon_mesh->vertices.push_back(Vec3(2.f * x * lvl->w / 16, h, 2.f * (y + 1) * lvl->h / 16));
				dungeon_mesh->vertices.push_back(Vec3(2.f * (x + 1) * lvl->w / 16, h, 2.f * (y + 1) * lvl->h / 16));
				dungeon_mesh->indices.push_back(index);
				dungeon_mesh->indices.push_back(index + 2);
				dungeon_mesh->indices.push_back(index + 1);
				dungeon_mesh->indices.push_back(index + 2);
				dungeon_mesh->indices.push_back(index + 3);
				dungeon_mesh->indices.push_back(index + 1);
				index += 4;
			}
		}
	}

	for(Room* room : lvl->rooms)
	{
		// floor
		dungeon_mesh->vertices.push_back(Vec3(2.f * room->pos.x, 0, 2.f * room->pos.y));
		dungeon_mesh->vertices.push_back(Vec3(2.f * (room->pos.x + room->size.x), 0, 2.f * room->pos.y));
		dungeon_mesh->vertices.push_back(Vec3(2.f * room->pos.x, 0, 2.f * (room->pos.y + room->size.y)));
		dungeon_mesh->vertices.push_back(Vec3(2.f * (room->pos.x + room->size.x), 0, 2.f * (room->pos.y + room->size.y)));
		dungeon_mesh->indices.push_back(index);
		dungeon_mesh->indices.push_back(index + 1);
		dungeon_mesh->indices.push_back(index + 2);
		dungeon_mesh->indices.push_back(index + 2);
		dungeon_mesh->indices.push_back(index + 1);
		dungeon_mesh->indices.push_back(index + 3);
		index += 4;

		// ceil
		const float h = (room->IsCorridor() ? Room::HEIGHT_LOW : Room::HEIGHT);
		dungeon_mesh->vertices.push_back(Vec3(2.f * room->pos.x, h, 2.f * room->pos.y));
		dungeon_mesh->vertices.push_back(Vec3(2.f * (room->pos.x + room->size.x), h, 2.f * room->pos.y));
		dungeon_mesh->vertices.push_back(Vec3(2.f * room->pos.x, h, 2.f * (room->pos.y + room->size.y)));
		dungeon_mesh->vertices.push_back(Vec3(2.f * (room->pos.x + room->size.x), h, 2.f * (room->pos.y + room->size.y)));
		dungeon_mesh->indices.push_back(index);
		dungeon_mesh->indices.push_back(index + 2);
		dungeon_mesh->indices.push_back(index + 1);
		dungeon_mesh->indices.push_back(index + 2);
		dungeon_mesh->indices.push_back(index + 3);
		dungeon_mesh->indices.push_back(index + 1);
		index += 4;
	}

	delete dungeon_shape;
	delete dungeon_shape_data;

	btIndexedMesh mesh;
	mesh.m_numTriangles = dungeon_mesh->indices.size() / 3;
	mesh.m_triangleIndexBase = (byte*)dungeon_mesh->indices.data();
	mesh.m_triangleIndexStride = sizeof(word) * 3;
	mesh.m_numVertices = dungeon_mesh->vertices.size();
	mesh.m_vertexBase = (byte*)dungeon_mesh->vertices.data();
	mesh.m_vertexStride = sizeof(Vec3);

	dungeon_shape_data = new btTriangleIndexVertexArray();
	dungeon_shape_data->addIndexedMesh(mesh, PHY_SHORT);
	dungeon_shape = new btBvhTriangleMeshShape(dungeon_shape_data, true);
	dungeon_shape->setUserPointer(dungeon_mesh);

	obj_dungeon = new btCollisionObject;
	obj_dungeon->setCollisionShape(dungeon_shape);
	obj_dungeon->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_BUILDING);
	phy_world->addCollisionObject(obj_dungeon, CG_BUILDING);
}

//=================================================================================================
void Level::SpawnDungeonCollider(const Vec3& pos)
{
	auto cobj = new btCollisionObject;
	cobj->setCollisionShape(shape_wall);
	cobj->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_BUILDING);
	cobj->getWorldTransform().setOrigin(ToVector3(pos));
	phy_world->addCollisionObject(cobj, CG_BUILDING);
}

//=================================================================================================
void Level::RemoveColliders()
{
	if(phy_world)
		phy_world->Reset();

	DeleteElements(shapes);
	cam_colliders.clear();
}

//=================================================================================================
Int2 Level::GetSpawnPoint()
{
	InsideLocation* inside = static_cast<InsideLocation*>(location);
	if(enter_from >= ENTER_FROM_PORTAL)
		return PosToPt(inside->GetPortal(enter_from)->GetSpawnPos());
	else if(enter_from == ENTER_FROM_NEXT_LEVEL)
		return lvl->GetNextEntryFrontTile();
	else
		return lvl->GetPrevEntryFrontTile();
}

//=================================================================================================
// Get nearest pos for unit to exit level
Vec3 Level::GetExitPos(Unit& u, bool force_border)
{
	const Vec3& pos = u.pos;

	if(location->outside)
	{
		if(u.area->area_type == LevelArea::Type::Building)
			return static_cast<InsideBuilding*>(u.area)->exit_region.Midpoint().XZ();
		else if(city_ctx && !force_border)
		{
			float best_dist, dist;
			int best_index = -1, index = 0;

			for(vector<EntryPoint>::const_iterator it = city_ctx->entry_points.begin(), end = city_ctx->entry_points.end(); it != end; ++it, ++index)
			{
				if(it->exit_region.IsInside(u.pos))
				{
					// unit is already inside exit area, goto outside exit
					best_index = -1;
					break;
				}
				else
				{
					dist = Vec2::Distance(Vec2(u.pos.x, u.pos.z), it->exit_region.Midpoint());
					if(best_index == -1 || dist < best_dist)
					{
						best_dist = dist;
						best_index = index;
					}
				}
			}

			if(best_index != -1)
				return city_ctx->entry_points[best_index].exit_region.Midpoint().XZ();
		}

		int best = 0;
		float dist, best_dist;

		// lewa kraw�d�
		best_dist = abs(pos.x - 32.f);

		// prawa kraw�d�
		dist = abs(pos.x - (256.f - 32.f));
		if(dist < best_dist)
		{
			best_dist = dist;
			best = 1;
		}

		// g�rna kraw�d�
		dist = abs(pos.z - 32.f);
		if(dist < best_dist)
		{
			best_dist = dist;
			best = 2;
		}

		// dolna kraw�d�
		dist = abs(pos.z - (256.f - 32.f));
		if(dist < best_dist)
			best = 3;

		switch(best)
		{
		default:
			assert(0);
		case 0:
			return Vec3(32.f, 0, pos.z);
		case 1:
			return Vec3(256.f - 32.f, 0, pos.z);
		case 2:
			return Vec3(pos.x, 0, 32.f);
		case 3:
			return Vec3(pos.x, 0, 256.f - 32.f);
		}
	}
	else
	{
		InsideLocation* inside = (InsideLocation*)location;
		if(dungeon_level == 0 && inside->from_portal)
			return inside->portal->pos;
		const Int2& pt = inside->GetLevelData().prevEntryPt;
		return PtToPos(pt);
	}
}

//=================================================================================================
bool Level::CanSee(Unit& u1, Unit& u2)
{
	if(u1.area != u2.area)
		return false;

	LevelArea& area = *u1.area;
	Int2 tile1(int(u1.pos.x / 2), int(u1.pos.z / 2)),
		tile2(int(u2.pos.x / 2), int(u2.pos.z / 2));

	if(area.area_type == LevelArea::Type::Outside)
	{
		OutsideLocation* outside = (OutsideLocation*)location;

		int xmin = max(0, min(tile1.x, tile2.x)),
			xmax = min(OutsideLocation::size, max(tile1.x, tile2.x)),
			ymin = max(0, min(tile1.y, tile2.y)),
			ymax = min(OutsideLocation::size, max(tile1.y, tile2.y));

		for(int y = ymin; y <= ymax; ++y)
		{
			for(int x = xmin; x <= xmax; ++x)
			{
				if(outside->tiles[x + y * OutsideLocation::size].IsBlocking()
					&& LineToRectangle(u1.pos, u2.pos, Vec2(2.f * x, 2.f * y), Vec2(2.f * (x + 1), 2.f * (y + 1))))
					return false;
			}
		}
	}
	else if(area.area_type == LevelArea::Type::Inside)
	{
		int xmin = max(0, min(tile1.x, tile2.x)),
			xmax = min(lvl->w, max(tile1.x, tile2.x)),
			ymin = max(0, min(tile1.y, tile2.y)),
			ymax = min(lvl->h, max(tile1.y, tile2.y));

		for(int y = ymin; y <= ymax; ++y)
		{
			for(int x = xmin; x <= xmax; ++x)
			{
				if(IsBlocking(lvl->map[x + y * lvl->w].type) && LineToRectangle(u1.pos, u2.pos, Vec2(2.f * x, 2.f * y), Vec2(2.f * (x + 1), 2.f * (y + 1))))
					return false;
				if(lvl->map[x + y * lvl->w].type == DOORS)
				{
					Door* door = area.FindDoor(Int2(x, y));
					if(door && door->IsBlocking())
					{
						Box2d box(door->pos.x, door->pos.z);
						if(door->rot == 0.f || door->rot == PI)
						{
							box.v1.x -= 1.f;
							box.v2.x += 1.f;
							box.v1.y -= Door::THICKNESS;
							box.v2.y += Door::THICKNESS;
						}
						else
						{
							box.v1.y -= 1.f;
							box.v2.y += 1.f;
							box.v1.x -= Door::THICKNESS;
							box.v2.x += Door::THICKNESS;
						}

						if(LineToRectangle(u1.pos, u2.pos, box.v1, box.v2))
							return false;
					}
				}
			}
		}
	}
	else
	{
		for(vector<CollisionObject>::iterator it = area.tmp->colliders.begin(), end = area.tmp->colliders.end(); it != end; ++it)
		{
			if(!it->cam_collider || it->type != CollisionObject::RECTANGLE)
				continue;

			Box2d box(it->pos.x - it->w, it->pos.z - it->h, it->pos.x + it->w, it->pos.z + it->h);
			if(LineToRectangle(u1.pos, u2.pos, box.v1, box.v2))
				return false;
		}

		for(vector<Door*>::iterator it = area.doors.begin(), end = area.doors.end(); it != end; ++it)
		{
			Door& door = **it;
			if(door.IsBlocking())
			{
				Box2d box(door.pos.x, door.pos.z);
				if(door.rot == 0.f || door.rot == PI)
				{
					box.v1.x -= 1.f;
					box.v2.x += 1.f;
					box.v1.y -= Door::THICKNESS;
					box.v2.y += Door::THICKNESS;
				}
				else
				{
					box.v1.y -= 1.f;
					box.v2.y += 1.f;
					box.v1.x -= Door::THICKNESS;
					box.v2.x += Door::THICKNESS;
				}

				if(LineToRectangle(u1.pos, u2.pos, box.v1, box.v2))
					return false;
			}
		}
	}

	return true;
}

//=================================================================================================
bool Level::CanSee(LevelArea& area, const Vec3& v1, const Vec3& v2, bool is_door, void* ignore)
{
	Int2 tile1(int(v1.x / 2), int(v1.z / 2)),
		tile2(int(v2.x / 2), int(v2.z / 2));

	if(area.area_type == LevelArea::Type::Outside)
	{
		OutsideLocation* outside = static_cast<OutsideLocation*>(location);

		int xmin = max(0, min(tile1.x, tile2.x)),
			xmax = min(OutsideLocation::size, max(tile1.x, tile2.x)),
			ymin = max(0, min(tile1.y, tile2.y)),
			ymax = min(OutsideLocation::size, max(tile1.y, tile2.y));

		for(int y = ymin; y <= ymax; ++y)
		{
			for(int x = xmin; x <= xmax; ++x)
			{
				if(outside->tiles[x + y * OutsideLocation::size].IsBlocking() && LineToRectangle(v1, v2, Vec2(2.f * x, 2.f * y), Vec2(2.f * (x + 1), 2.f * (y + 1))))
					return false;
			}
		}
	}
	else if(area.area_type == LevelArea::Type::Inside)
	{
		int xmin = max(0, min(tile1.x, tile2.x)),
			xmax = min(lvl->w, max(tile1.x, tile2.x)),
			ymin = max(0, min(tile1.y, tile2.y)),
			ymax = min(lvl->h, max(tile1.y, tile2.y));

		for(int y = ymin; y <= ymax; ++y)
		{
			for(int x = xmin; x <= xmax; ++x)
			{
				if(IsBlocking(lvl->map[x + y * lvl->w].type) && LineToRectangle(v1, v2, Vec2(2.f * x, 2.f * y), Vec2(2.f * (x + 1), 2.f * (y + 1))))
					return false;
				if(lvl->map[x + y * lvl->w].type == DOORS)
				{
					Int2 pt(x, y);
					Door* door = area.FindDoor(pt);
					if(door && door->IsBlocking()
						&& (!is_door || tile2 != pt)) // ignore target door
					{
						Box2d box(door->pos.x, door->pos.z);
						if(door->rot == 0.f || door->rot == PI)
						{
							box.v1.x -= 1.f;
							box.v2.x += 1.f;
							box.v1.y -= Door::THICKNESS;
							box.v2.y += Door::THICKNESS;
						}
						else
						{
							box.v1.y -= 1.f;
							box.v2.y += 1.f;
							box.v1.x -= Door::THICKNESS;
							box.v2.x += Door::THICKNESS;
						}

						if(LineToRectangle(v1, v2, box.v1, box.v2))
							return false;
					}
				}
			}
		}
	}
	else
	{
		for(vector<CollisionObject>::iterator it = area.tmp->colliders.begin(), end = area.tmp->colliders.end(); it != end; ++it)
		{
			if(!it->cam_collider || it->type != CollisionObject::RECTANGLE || (ignore && ignore == it->owner))
				continue;

			Box2d box(it->pos.x - it->w, it->pos.z - it->h, it->pos.x + it->w, it->pos.z + it->h);
			if(LineToRectangle(v1, v2, box.v1, box.v2))
				return false;
		}

		for(vector<Door*>::iterator it = area.doors.begin(), end = area.doors.end(); it != end; ++it)
		{
			Door& door = **it;
			if(door.IsBlocking()
				&& (!is_door || !v2.Equal(door.pos))) // ignore target door
			{
				Box2d box(door.pos.x, door.pos.z);
				if(door.rot == 0.f || door.rot == PI)
				{
					box.v1.x -= 1.f;
					box.v2.x += 1.f;
					box.v1.y -= Door::THICKNESS;
					box.v2.y += Door::THICKNESS;
				}
				else
				{
					box.v1.y -= 1.f;
					box.v2.y += 1.f;
					box.v1.x -= Door::THICKNESS;
					box.v2.x += Door::THICKNESS;
				}

				if(LineToRectangle(v1, v2, box.v1, box.v2))
					return false;
			}
		}
	}

	return true;
}

//=================================================================================================
bool Level::KillAll(int mode, Unit& unit, Unit* ignore)
{
	if(!InRange(mode, 0, 1))
		return false;

	if(!Net::IsLocal())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::CHEAT_KILLALL;
		c.id = mode;
		c.unit = ignore;
		return true;
	}

	switch(mode)
	{
	case 0: // kill enemies
		for(LevelArea& area : ForEachArea())
		{
			for(Unit* u : area.units)
			{
				if(u->IsAlive() && u->IsEnemy(unit) && u != ignore)
					u->GiveDmg(u->hp);
			}
		}
		break;
	case 1: // kill all except player/ignore
		for(LevelArea& area : ForEachArea())
		{
			for(Unit* u : area.units)
			{
				if(u->IsAlive() && !u->IsPlayer() && u != ignore)
					u->GiveDmg(u->hp);
			}
		}
		break;
	}

	return true;
}

//=================================================================================================
void Level::AddPlayerTeam(const Vec3& pos, float rot)
{
	const bool hide_weapon = enter_from == ENTER_FROM_OUTSIDE;

	for(Unit& unit : team->members)
	{
		local_area->units.push_back(&unit);
		unit.CreatePhysics();
		if(unit.IsHero())
			game->ais.push_back(unit.ai);

		unit.SetTakeHideWeaponAnimationToEnd(hide_weapon, false);
		unit.rot = rot;
		unit.animation = unit.current_animation = ANI_STAND;
		unit.mesh_inst->Play(NAMES::ani_stand, PLAY_PRIO1, 0);
		unit.BreakAction();
		unit.SetAnimationAtEnd();
		unit.area = local_area;

		if(unit.IsAI())
		{
			unit.ai->state = AIController::Idle;
			unit.ai->st.idle.action = AIController::Idle_None;
			unit.ai->target = nullptr;
			unit.ai->alert_target = nullptr;
			unit.ai->timer = Random(2.f, 5.f);
		}

		WarpNearLocation(*local_area, unit, pos, city_ctx ? 4.f : 2.f, true, 20);
		unit.visual_pos = unit.pos;

		if(!location->outside)
			FOV::DungeonReveal(Int2(int(unit.pos.x / 2), int(unit.pos.z / 2)), minimap_reveal);

		if(unit.interp)
			unit.interp->Reset(unit.pos, unit.rot);
	}
}

//=================================================================================================
void Level::UpdateDungeonMinimap(bool in_level)
{
	if(minimap_opened_doors)
	{
		for(Unit& unit : team->active_members)
		{
			if(unit.IsPlayer())
				FOV::DungeonReveal(Int2(int(unit.pos.x / 2), int(unit.pos.z / 2)), minimap_reveal);
		}
	}

	if(minimap_reveal.empty())
		return;

	DynamicTexture& tex = *game->tMinimap;
	tex.Lock();
	for(vector<Int2>::iterator it = minimap_reveal.begin(), end = minimap_reveal.end(); it != end; ++it)
	{
		Tile& p = lvl->map[it->x + (lvl->w - it->y - 1) * lvl->w];
		SetBit(p.flags, Tile::F_REVEALED);
		uint* pix = tex[it->y] + it->x;
		if(OR2_EQ(p.type, WALL, BLOCKADE_WALL))
			*pix = Color(100, 100, 100);
		else if(p.type == DOORS)
			*pix = Color(127, 51, 0);
		else
			*pix = Color(220, 220, 240);
	}
	tex.Unlock();

	if(Net::IsLocal())
	{
		if(in_level)
		{
			if(Net::IsOnline())
				minimap_reveal_mp.insert(minimap_reveal_mp.end(), minimap_reveal.begin(), minimap_reveal.end());
		}
		else
			minimap_reveal_mp.clear();
	}

	minimap_reveal.clear();
}

//=================================================================================================
void Level::RevealMinimap()
{
	if(location->outside)
		return;

	for(int y = 0; y < lvl->h; ++y)
	{
		for(int x = 0; x < lvl->w; ++x)
			minimap_reveal.push_back(Int2(x, y));
	}

	UpdateDungeonMinimap(false);

	if(Net::IsServer())
		Net::PushChange(NetChange::CHEAT_REVEAL_MINIMAP);
}

//=================================================================================================
bool Level::IsCity()
{
	return location->type == L_CITY && location->target != VILLAGE;
}

//=================================================================================================
bool Level::IsVillage()
{
	return location->type == L_CITY && location->target == VILLAGE;
}

//=================================================================================================
bool Level::IsTutorial()
{
	return location->type == L_DUNGEON && location->target == TUTORIAL_FORT;
}

//=================================================================================================
void Level::Update()
{
	for(LevelArea& area : ForEachArea())
	{
		for(Unit* unit : area.units)
		{
			if(unit->data->trader)
				unit->RefreshStock();
		}
	}
}

//=================================================================================================
void Level::Write(BitStreamWriter& f)
{
	location->Write(f);
	f.WriteCasted<byte>(GetLocationMusic());
	f << boss;

	if(net->mp_load)
	{
		for(LevelArea& area : ForEachArea())
			area.tmp->Write(f);
	}
}

//=================================================================================================
bool Level::Read(BitStreamReader& f, bool loaded_resources)
{
	// location
	if(!location->Read(f))
	{
		Error("Read level: Failed to read location.");
		return false;
	}
	is_open = true;
	Apply();
	game->loc_gen_factory->Get(location)->OnLoad();
	location->RequireLoadingResources(&loaded_resources);

	// apply usable users
	Usable::ApplyRequests();

	// music
	MusicType music;
	f.ReadCasted<byte>(music);
	if(!f)
	{
		Error("Read level: Broken music.");
		return false;
	}
	game_res->LoadMusic(music, false, true);
	f >> boss;
	if(boss)
	{
		game_res->LoadMusic(MusicType::Boss, false, true);
		game->SetMusic(MusicType::Boss);
	}
	else
		game->SetMusic(music);

	if(net->mp_load)
	{
		for(LevelArea& area : ForEachArea())
		{
			if(!area.tmp->Read(f))
				return false;
			area.RecreateScene();
		}
	}

	return true;
}

//=================================================================================================
MusicType Level::GetLocationMusic()
{
	switch(location->type)
	{
	case L_CITY:
		return MusicType::City;
	case L_DUNGEON:
	case L_CAVE:
		if(Any(location->target, HERO_CRYPT, MONSTER_CRYPT))
			return MusicType::Crypt;
		else
			return MusicType::Dungeon;
	case L_OUTSIDE:
		if(location_index == quest_mgr->quest_secret->where2 || Any(location->target, MOONWELL, ACADEMY))
			return MusicType::Moonwell;
		else
			return MusicType::Forest;
	case L_CAMP:
		return MusicType::Forest;
	case L_ENCOUNTER:
		return MusicType::Travel;
	default:
		assert(0);
		return MusicType::Dungeon;
	}
}

//=================================================================================================
// -1 - outside
// -2 - all
void Level::CleanLevel(int building_id)
{
	for(LevelArea& area : ForEachArea())
	{
		if((area.area_type != LevelArea::Type::Building && (building_id == -2 || building_id == -1))
			|| (area.area_type == LevelArea::Type::Building && (building_id == -2 || building_id == area.area_id)))
		{
			DeleteElements(area.bloods);
			for(Unit* unit : area.units)
			{
				if(!unit->IsAlive() && !unit->IsTeamMember() && !unit->to_remove)
					RemoveUnit(unit, false);
			}
		}
	}

	if(Net::IsServer())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::CLEAN_LEVEL;
		c.id = building_id;
	}
}

//=================================================================================================
GroundItem* Level::SpawnItem(const Item* item, const Vec3& pos)
{
	GroundItem* gi = new GroundItem;
	gi->Register();
	gi->count = 1;
	gi->team_count = 1;
	gi->rot = Quat::RotY(Random(MAX_ANGLE));
	gi->pos = pos;
	if(local_area->area_type == LevelArea::Type::Outside)
		terrain->SetY(gi->pos);
	gi->item = item;
	local_area->items.push_back(gi);
	return gi;
}

//=================================================================================================
GroundItem* Level::SpawnItemAtObject(const Item* item, Object* obj)
{
	assert(item && obj);
	Mesh* mesh = obj->base->mesh;
	GroundItem* ground_item = SpawnItem(item, obj->pos);
	for(vector<Mesh::Point>::iterator it = mesh->attach_points.begin(), end = mesh->attach_points.end(); it != end; ++it)
	{
		if(strncmp(it->name.c_str(), "spawn_", 6) == 0)
		{
			Matrix rot = Matrix::RotationY(obj->rot.y + PI);
			Vec3 offset = Vec3::TransformZero(rot * it->mat);
			ground_item->pos += offset;
			ground_item->rot = Quat::CreateFromRotationMatrix(it->mat) * Quat::RotY(obj->rot.y);
			break;
		}
	}
	return ground_item;
}

//=================================================================================================
void Level::SpawnItemRandomly(const Item* item, uint count)
{
	assert(location->outside);
	for(uint i = 0; i < count; ++i)
	{
		const float s = (float)OutsideLocation::size;
		Vec2 pos = Vec2(s, s) + Vec2::RandomPoissonDiscPoint() * (s - 30.f);
		SpawnGroundItemInsideRadius(item, pos, 3.f, true);
	}
}

//=================================================================================================
Unit* Level::GetNearestEnemy(Unit* unit)
{
	Unit* best = nullptr;
	float best_dist;
	for(Unit* u : unit->area->units)
	{
		if(u->IsAlive() && unit->IsEnemy(*u))
		{
			float dist = Vec3::DistanceSquared(unit->pos, u->pos);
			if(!best || dist < best_dist)
			{
				best = u;
				best_dist = dist;
			}
		}
	}
	return best;
}

//=================================================================================================
Unit* Level::SpawnUnitNearLocationS(UnitData* ud, const Vec3& pos, float range)
{
	return SpawnUnitNearLocation(GetArea(pos), pos, *ud, nullptr, -1, range);
}

//=================================================================================================
GroundItem* Level::FindNearestItem(const Item* item, const Vec3& pos)
{
	LevelArea& area = GetArea(pos);
	float best_dist = 999.f;
	GroundItem* best_item = nullptr;
	for(GroundItem* ground_item : area.items)
	{
		if(ground_item->item == item)
		{
			float dist = Vec3::Distance(pos, ground_item->pos);
			if(dist < best_dist || !best_item)
			{
				best_dist = dist;
				best_item = ground_item;
			}
		}
	}
	return best_item;
}

//=================================================================================================
GroundItem* Level::FindItem(const Item* item)
{
	for(LevelArea& area : ForEachArea())
	{
		for(GroundItem* ground_item : area.items)
		{
			if(ground_item->item == item)
				return ground_item;
		}
	}
	return nullptr;
}

//=================================================================================================
Unit* Level::GetMayor()
{
	if(!city_ctx)
		return nullptr;
	cstring id;
	if(city_ctx->target == VILLAGE)
		id = "soltys";
	else
		id = "mayor";
	return FindUnit(UnitData::Get(id));
}

//=================================================================================================
bool Level::IsSafe()
{
	if(city_ctx)
		return true;
	else if(location->outside)
		return location->state == LS_CLEARED || location->type == L_ENCOUNTER;
	else
	{
		InsideLocation* inside = static_cast<InsideLocation*>(location);
		if(inside->IsMultilevel())
		{
			MultiInsideLocation* multi = static_cast<MultiInsideLocation*>(inside);
			if(multi->IsLevelClear())
			{
				if(dungeon_level == 0)
				{
					if(!multi->from_portal)
						return true;
				}
				else
					return true;
			}
			return false;
		}
		else
			return (location->state == LS_CLEARED);
	}
}

//=================================================================================================
bool Level::CanFastTravel()
{
	if(Net::IsLocal())
	{
		if(!location->outside
			|| !IsSafe()
			|| game->arena->mode != Arena::NONE
			|| quest_mgr->quest_tutorial->in_tutorial
			|| quest_mgr->quest_contest->state >= Quest_Contest::CONTEST_STARTING
			|| quest_mgr->quest_tournament->GetState() != Quest_Tournament::TOURNAMENT_NOT_DONE)
			return false;

		CanLeaveLocationResult result = CanLeaveLocation(*team->leader, false);
		return result == CanLeaveLocationResult::Yes;
	}
	else
		return can_fast_travel;
}

//=================================================================================================
CanLeaveLocationResult Level::CanLeaveLocation(Unit& unit, bool check_dist)
{
	if(quest_mgr->quest_secret->state == Quest_Secret::SECRET_FIGHT)
		return CanLeaveLocationResult::InCombat;

	if(city_ctx)
	{
		for(Unit& u : team->members)
		{
			if(u.summoner)
				continue;

			if(u.busy != Unit::Busy_No && u.busy != Unit::Busy_Tournament)
				return CanLeaveLocationResult::TeamTooFar;

			if(u.IsPlayer() && check_dist)
			{
				if(u.area->area_type == LevelArea::Type::Building || Vec3::Distance2d(unit.pos, u.pos) > 8.f)
					return CanLeaveLocationResult::TeamTooFar;
			}

			for(vector<Unit*>::iterator it2 = local_area->units.begin(), end2 = local_area->units.end(); it2 != end2; ++it2)
			{
				Unit& u2 = **it2;
				if(&u != &u2 && u2.IsStanding() && unit.IsEnemy(u2) && u2.IsAI() && u2.ai->in_combat
					&& Vec3::Distance2d(unit.pos, u2.pos) < ALERT_RANGE && CanSee(u, u2))
					return CanLeaveLocationResult::InCombat;
			}
		}
	}
	else
	{
		for(Unit& u : team->members)
		{
			if(u.summoner)
				continue;

			if(u.busy != Unit::Busy_No || (check_dist && Vec3::Distance2d(unit.pos, u.pos) > 8.f))
				return CanLeaveLocationResult::TeamTooFar;

			for(vector<Unit*>::iterator it2 = local_area->units.begin(), end2 = local_area->units.end(); it2 != end2; ++it2)
			{
				Unit& u2 = **it2;
				if(&u != &u2 && u2.IsStanding() && u.IsEnemy(u2) && u2.IsAI() && u2.ai->in_combat
					&& Vec3::Distance2d(u.pos, u2.pos) < ALERT_RANGE && CanSee(u, u2))
					return CanLeaveLocationResult::InCombat;
			}
		}
	}

	return CanLeaveLocationResult::Yes;
}

//=================================================================================================
void Level::SetOutsideParams()
{
	camera.zfar = 80.f;
	game->clear_color_next = Color::White;
	scene->fog_range = Vec2(40, 80);
	scene->fog_color = Color(0.9f, 0.85f, 0.8f);
	scene->ambient_color = Color(0.5f, 0.5f, 0.5f);
	scene->light_color = Color::White;
	scene->light_dir = Vec3(sin(light_angle), 2.f, cos(light_angle)).Normalize();
	scene->use_light_dir = true;
}

//=================================================================================================
bool Level::CanShootAtLocation(const Vec3& from, const Vec3& to) const
{
	RaytestAnyUnitCallback callback;
	phy_world->rayTest(ToVector3(from), ToVector3(to), callback);
	return callback.clear;
}

//=================================================================================================
bool Level::CanShootAtLocation2(const Unit& me, const void* ptr, const Vec3& to) const
{
	RaytestWithIgnoredCallback callback(&me, ptr);
	phy_world->rayTest(btVector3(me.pos.x, me.pos.y + 1.f, me.pos.z), btVector3(to.x, to.y + 1.f, to.z), callback);
	return !callback.hit;
}

//=================================================================================================
bool Level::RayTest(const Vec3& from, const Vec3& to, Unit* ignore, Vec3& hitpoint, Unit*& hitted)
{
	RaytestClosestUnitCallback callback([=](Unit* unit)
	{
		if(unit == ignore || !unit->IsAlive())
			return false;
		return true;
	});
	phy_world->rayTest(ToVector3(from), ToVector3(to), callback);

	if(callback.hasHit())
	{
		hitpoint = from + (to - from) * callback.getFraction();
		hitted = callback.hit;
		return true;
	}
	else
		return false;
}

//=================================================================================================
bool Level::LineTest(btCollisionShape* shape, const Vec3& from, const Vec3& dir, delegate<LINE_TEST_RESULT(btCollisionObject*, bool)> clbk, float& t,
	vector<float>* t_list, bool use_clbk2, float* end_t)
{
	assert(shape->isConvex());

	btTransform t_from, t_to;
	t_from.setIdentity();
	t_from.setOrigin(ToVector3(from));
	t_to.setIdentity();
	t_to.setOrigin(ToVector3(dir) + t_from.getOrigin());

	ConvexCallback callback(clbk, t_list, use_clbk2);

	phy_world->convexSweepTest((btConvexShape*)shape, t_from, t_to, callback);

	bool has_hit = (callback.closest <= 1.f);
	t = min(callback.closest, 1.f);
	if(end_t)
	{
		if(callback.end)
			*end_t = callback.end_t;
		else
			*end_t = 1.f;
	}
	return has_hit;
}

//=================================================================================================
bool Level::ContactTest(btCollisionObject* obj, delegate<bool(btCollisionObject*, bool)> clbk, bool use_clbk2)
{
	ContactTestCallback callback(obj, clbk, use_clbk2);
	phy_world->contactTest(obj, callback);
	return callback.hit;
}

//=================================================================================================
int Level::CheckMove(Vec3& pos, const Vec3& dir, float radius, Unit* me, bool* is_small)
{
	assert(radius > 0.f && me);

	constexpr float SMALL_DISTANCE = 0.001f;
	Vec3 new_pos = pos + dir;
	Vec3 gather_pos = pos + dir / 2;
	float gather_radius = dir.Length() + radius;
	global_col.clear();

	Level::IgnoreObjects ignore = { 0 };
	Unit* ignored[] = { me, nullptr };
	ignore.ignored_units = (const Unit**)ignored;
	GatherCollisionObjects(*me->area, global_col, gather_pos, gather_radius, &ignore);

	if(global_col.empty())
	{
		if(is_small)
			*is_small = (Vec3::Distance(pos, new_pos) < SMALL_DISTANCE);
		pos = new_pos;
		return 3;
	}

	// id� prosto po x i z
	if(!Collide(global_col, new_pos, radius))
	{
		if(is_small)
			*is_small = (Vec3::Distance(pos, new_pos) < SMALL_DISTANCE);
		pos = new_pos;
		return 3;
	}

	// id� po x
	Vec3 new_pos2 = me->pos;
	new_pos2.x = new_pos.x;
	if(!Collide(global_col, new_pos2, radius))
	{
		if(is_small)
			*is_small = (Vec3::Distance(pos, new_pos2) < SMALL_DISTANCE);
		pos = new_pos2;
		return 1;
	}

	// id� po z
	new_pos2.x = me->pos.x;
	new_pos2.z = new_pos.z;
	if(!Collide(global_col, new_pos2, radius))
	{
		if(is_small)
			*is_small = (Vec3::Distance(pos, new_pos2) < SMALL_DISTANCE);
		pos = new_pos2;
		return 2;
	}

	// nie ma drogi
	return 0;
}

//=================================================================================================
void Level::SpawnUnitEffect(Unit& unit)
{
	Vec3 real_pos = unit.pos;
	real_pos.y += 1.f;
	sound_mgr->PlaySound3d(game_res->sSummon, real_pos, SPAWN_SOUND_DIST);

	ParticleEmitter* pe = new ParticleEmitter;
	pe->tex = game_res->tSpawn;
	pe->emision_interval = 0.1f;
	pe->life = 5.f;
	pe->particle_life = 0.5f;
	pe->emisions = 5;
	pe->spawn_min = 10;
	pe->spawn_max = 15;
	pe->max_particles = 15 * 5;
	pe->pos = unit.pos;
	pe->speed_min = Vec3(-1, 0, -1);
	pe->speed_max = Vec3(1, 1, 1);
	pe->pos_min = Vec3(-0.75f, 0, -0.75f);
	pe->pos_max = Vec3(0.75f, 1.f, 0.75f);
	pe->size = 0.3f;
	pe->op_size = ParticleEmitter::POP_LINEAR_SHRINK;
	pe->alpha = 0.5f;
	pe->op_alpha = ParticleEmitter::POP_LINEAR_SHRINK;
	pe->mode = 0;
	pe->Init();
	unit.area->tmp->pes.push_back(pe);
}

//=================================================================================================
MeshInstance* Level::GetBowInstance(Mesh* mesh)
{
	if(bow_instances.empty())
	{
		if(!mesh->IsLoaded())
			res_mgr->LoadInstant(mesh);
		return new MeshInstance(mesh);
	}
	else
	{
		MeshInstance* instance = bow_instances.back();
		bow_instances.pop_back();
		instance->mesh = mesh;
		return instance;
	}
}

//=================================================================================================
CityBuilding* Level::GetRandomBuilding(BuildingGroup* group)
{
	assert(group);
	if(!city_ctx)
		return nullptr;
	LocalVector<CityBuilding*> available;
	for(CityBuilding& building : city_ctx->buildings)
	{
		if(building.building->group == group)
			available.push_back(&building);
	}
	if(available.empty())
		return nullptr;
	return available.RandomItem();
}

//=================================================================================================
Room* Level::GetRoom(RoomTarget target)
{
	if(location->outside)
		return nullptr;
	for(Room* room : lvl->rooms)
	{
		if(room->target == target)
			return room;
	}
	return nullptr;
}

//=================================================================================================
Object* Level::FindObjectInRoom(Room& room, BaseObject* base)
{
	for(Object* obj : local_area->objects)
	{
		if(obj->base == base && room.IsInside(obj->pos))
			return obj;
	}
	return nullptr;
}

//=================================================================================================
CScriptArray* Level::FindPath(Room& from, Room& to)
{
	assert(lvl);

	asITypeInfo* type = script_mgr->GetEngine()->GetTypeInfoByDecl("array<Room@>");
	CScriptArray* array = CScriptArray::Create(type);

	Int2 from_pt = from.CenterTile();
	Int2 to_pt = to.CenterTile();

	vector<Int2> path;
	pathfinding->FindPath(*local_area, from_pt, to_pt, path);
	std::reverse(path.begin(), path.end());

	word prev_room_index = 0xFFFF;
	for(const Int2& pt : path)
	{
		word room_index = lvl->map[pt.x + pt.y * lvl->w].room;
		if(room_index != prev_room_index)
		{
			Room* room = lvl->rooms[room_index];
			array->InsertLast(&room);
			prev_room_index = room_index;
		}
	}

	return array;
}

//=================================================================================================
CScriptArray* Level::GetUnits(Room& room)
{
	assert(lvl);

	asITypeInfo* type = script_mgr->GetEngine()->GetTypeInfoByDecl("array<Unit@>");
	CScriptArray* array = CScriptArray::Create(type);

	for(Unit* unit : local_area->units)
	{
		if(room.IsInside(unit->pos))
			array->InsertLast(&unit);
	}

	return array;
}

//=================================================================================================
bool Level::FindPlaceNearWall(BaseObject& obj, SpawnPoint& point)
{
	assert(lvl);

	Mesh::Point* pt = obj.mesh->FindPoint("hit");
	assert(pt);
	const Vec2 size = pt->size.XZ() * 2;

	const int width = 2 + (int)ceil(size.x / 2);
	const int width_a = (width - 1) / 2;
	const int width_b = width - width_a - 1;
	const int start_x = Random(width_a, lvl->h - width_b);
	const int start_y = Random(width_a, lvl->h - width_b);
	int x = start_x, y = start_y;

	while(true)
	{
		// __#
		// _?#
		// __#
		bool ok = true;
		for(int y1 = -width_a; y1 <= width_b; ++y1)
		{
			if(IsBlocking2(lvl->map[x - 1 + (y + y1) * lvl->w])
				|| IsBlocking2(lvl->map[x + (y + y1) * lvl->w])
				|| lvl->map[x + 1 + (y + y1) * lvl->w].type != WALL)
			{
				ok = false;
				break;
			}
		}
		if(ok)
		{
			point.pos = Vec3(2.f * (x + 1), 0, 2.f * (y - width_a) + (float)width);
			point.rot = DirToRot(GDIR_LEFT);
			return true;
		}

		// #__
		// #?_
		// #__
		ok = true;
		for(int y1 = -width_a; y1 <= width_b; ++y1)
		{
			if(IsBlocking2(lvl->map[x + 1 + (y + y1) * lvl->w])
				|| IsBlocking2(lvl->map[x + (y + y1) * lvl->w])
				|| lvl->map[x - 1 + (y + y1) * lvl->w].type != WALL)
			{
				ok = false;
				break;
			}
		}
		if(ok)
		{
			point.pos = Vec3(2.f * x, 0, 2.f * (y - width_a) + (float)width);
			point.rot = DirToRot(GDIR_RIGHT);
			return true;
		}

		// ###
		// _?_
		// ___
		ok = true;
		for(int x1 = -width_a; x1 <= width_b; ++x1)
		{
			if(IsBlocking2(lvl->map[x + x1 + (y - 1) * lvl->w])
				|| IsBlocking2(lvl->map[x + x1 + y * lvl->w])
				|| lvl->map[x + x1 + (y + 1) * lvl->w].type != WALL)
			{
				ok = false;
				break;
			}
		}
		if(ok)
		{
			point.pos = Vec3(2.f * (x - width_a) + (float)width, 0, 2.f * (y + 1));
			point.rot = DirToRot(GDIR_DOWN);
			return true;
		}

		// ___
		// _?_
		// ###
		ok = true;
		for(int x1 = -width_a; x1 <= width_b; ++x1)
		{
			if(IsBlocking2(lvl->map[x + x1 + (y + 1) * lvl->w])
				|| IsBlocking2(lvl->map[x + x1 + y * lvl->w])
				|| lvl->map[x + x1 + (y - 1) * lvl->w].type != WALL)
			{
				ok = false;
				break;
			}
		}
		if(ok)
		{
			point.pos = Vec3(2.f * (x - width_a) + (float)width, 0, 2.f * y);
			point.rot = DirToRot(GDIR_UP);
			return true;
		}

		++x;
		if(x == lvl->w - width_b)
		{
			x = width_a;
			++y;
			if(y == lvl->h - width_b)
				y = width_a;
		}
		if(x == start_x && y == start_y)
			return false;
	}
}

//=================================================================================================
void Level::CreateObjectsMeshInstance()
{
	const bool isLoading = (game->in_load || net->mp_load);
	for(LevelArea& area : ForEachArea())
	{
		for(Object* obj : area.objects)
		{
			if(obj->mesh->IsAnimated())
			{
				float time = 0;
				if(isLoading)
					time = obj->time;
				obj->meshInst = new MeshInstance(obj->mesh);
				obj->meshInst->Play(&obj->mesh->anims[0], PLAY_NO_BLEND, 0);
				if(time != 0)
					obj->meshInst->groups[0].time = time;
			}
		}

		for(Chest* chest : area.chests)
			chest->Recreate();
	}
}

//=================================================================================================
void Level::RemoveTmpObjectPhysics()
{
	for(LevelArea& area : ForEachArea())
	{
		LoopAndRemove(area.tmp->colliders, [](const CollisionObject& cobj)
		{
			return cobj.owner == CollisionObject::TMP;
		});
	}
}

//=================================================================================================
void Level::RecreateTmpObjectPhysics()
{
	for(LevelArea& area : ForEachArea())
	{
		for(Object* obj : area.objects)
		{
			if(!obj->base || !IsSet(obj->base->flags, OBJ_TMP_PHYSICS))
				continue;
			CollisionObject& c = Add1(area.tmp->colliders);
			c.owner = CollisionObject::TMP;
			c.cam_collider = false;
			if(obj->base->type == OBJ_CYLINDER)
			{
				c.type = CollisionObject::SPHERE;
				c.pos = obj->pos;
				c.radius = obj->base->r;
			}
			else if(obj->base->type == OBJ_HITBOX)
			{
				const Matrix mat = *obj->base->matrix * Matrix::RotationY(obj->rot.y);
				Vec3 pos2 = Vec3::TransformZero(mat);
				pos2 += obj->pos;

				c.pos = pos2;
				c.w = obj->base->size.x;
				c.h = obj->base->size.y;
				if(NotZero(obj->rot.y))
				{
					c.type = CollisionObject::RECTANGLE_ROT;
					c.rot = obj->rot.y;
					c.radius = max(c.w, c.h) * SQRT_2;
				}
				else
					c.type = CollisionObject::RECTANGLE;
			}
		}
	}
}

//=================================================================================================
void Level::StartBossFight(Unit& unit)
{
	boss = &unit;
	game_gui->level_gui->SetBoss(&unit, false);
	game->SetMusic(MusicType::Boss);
	if(Net::IsServer())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::BOSS_START;
		c.unit = &unit;
	}
}

//=================================================================================================
void Level::EndBossFight()
{
	boss = nullptr;
	game_gui->level_gui->SetBoss(nullptr, false);
	game->SetMusic();
	if(Net::IsServer())
		Net::PushChange(NetChange::BOSS_END);
}

//=================================================================================================
void Level::RecreateScene()
{
	for(LevelArea& area : ForEachArea())
		area.RecreateScene();
}
