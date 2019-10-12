#include "Pch.h"
#include "GameCore.h"
#include "Level.h"
#include "City.h"
#include "InsideBuilding.h"
#include "MultiInsideLocation.h"
#include "InsideLocationLevel.h"
#include "Door.h"
#include "Trap.h"
#include "Chest.h"
#include "UnitEventHandler.h"
#include "AIController.h"
#include "ResourceManager.h"
#include "Terrain.h"
#include "UnitGroup.h"
#include "EntityInterpolator.h"
#include "Arena.h"
#include "Game.h"
#include "ParticleSystem.h"
#include "Language.h"
#include "World.h"
#include "Portal.h"
#include "Team.h"
#include "FOV.h"
#include "Texture.h"
#include "PlayerInfo.h"
#include "BitStreamFunc.h"
#include "Spell.h"
#include "QuestManager.h"
#include "Quest_Secret.h"
#include "Quest_Tutorial.h"
#include "Quest_Contest.h"
#include "Quest_Tournament.h"
#include "LocationGeneratorFactory.h"
#include "SoundManager.h"
#include "Render.h"
#include "SpellEffects.h"
#include "Collision.h"
#include "LocationHelper.h"
#include "PhysicCallbacks.h"
#include "GameResources.h"

Level* global::game_level;

//=================================================================================================
Level::Level() : local_area(nullptr), terrain(nullptr), terrain_shape(nullptr), dungeon_shape(nullptr), dungeon_shape_data(nullptr), shape_wall(nullptr),
shape_stairs(nullptr), shape_stairs_part(), shape_block(nullptr), shape_barrier(nullptr), shape_door(nullptr), shape_arrow(nullptr), shape_summon(nullptr),
cl_fog(true), cl_lighting(true)
{
	camera.draw_range = 80.f;
}

//=================================================================================================
Level::~Level()
{
	DeleteElements(bow_instances);

	delete terrain;
	delete terrain_shape;
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
void Level::LoadData()
{
	tFlare = res_mgr->Load<Texture>("flare.png");
	tFlare2 = res_mgr->Load<Texture>("flare2.png");
	tWater = res_mgr->Load<Texture>("water.png");
}

//=================================================================================================
void Level::Init()
{
	terrain = new Terrain;
	TerrainOptions terrain_options;
	terrain_options.n_parts = 8;
	terrain_options.tex_size = 256;
	terrain_options.tile_size = 2.f;
	terrain_options.tiles_per_part = 16;
	terrain->Init(render->GetDevice(), terrain_options);
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

	Mesh::Point* point = game_res->aArrow->FindPoint("Empty");
	assert(point && point->IsBox());
	shape_arrow = new btBoxShape(ToVector3(point->size));
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
			warp.unit->rot = building.outside_rot;
			WarpUnit(*warp.unit, building.outside_spawn);
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
			area.tmp = TmpLevelArea::Get();
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
ObjectEntity Level::SpawnObjectEntity(LevelArea& area, BaseObject* base, const Vec3& pos, float rot, float scale, int flags, Vec3* out_point,
	int variant)
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
			u->pos = pos + Vec3(sin(sdir)*slen, 0, cos(sdir)*slen);
			u->rot = sdir;
			area.usables.push_back(u);

			SpawnObjectExtras(area, stool, u->pos, u->rot, u);
		}

		return o;
	}
	else if(IsSet(base->flags, OBJ_BUILDING))
	{
		// building
		int roti;
		if(Equal(rot, 0))
			roti = 0;
		else if(Equal(rot, PI / 2))
			roti = 1;
		else if(Equal(rot, PI))
			roti = 2;
		else if(Equal(rot, PI * 3 / 2))
			roti = 3;
		else
		{
			assert(0);
			roti = 0;
			rot = 0.f;
		}

		Object* o = new Object;
		o->mesh = base->mesh;
		o->rot = Vec3(0, rot, 0);
		o->pos = pos;
		o->scale = scale;
		o->base = base;
		area.objects.push_back(o);

		ProcessBuildingObjects(area, nullptr, nullptr, o->mesh, nullptr, rot, roti, pos, nullptr, nullptr, false, out_point);

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
		chest->mesh_inst = new MeshInstance(base->mesh);
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

	// ogieñ pochodni
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
			pe->op_alpha = POP_LINEAR_SHRINK;
			pe->op_size = POP_LINEAR_SHRINK;
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

			pe->tex = tFlare;
			if(IsSet(obj->flags, OBJ_CAMPFIRE_EFFECT))
				pe->size = 0.7f;
			else
			{
				pe->size = 0.5f;
				if(IsSet(flags, SOE_MAGIC_LIGHT))
					pe->tex = tFlare2;
			}

			// œwiat³o
			if(!IsSet(flags, SOE_DONT_CREATE_LIGHT))
			{
				Light& s = Add1(area.lights);
				s.pos = pe->pos;
				s.range = 5;
				if(IsSet(flags, SOE_MAGIC_LIGHT))
					s.color = Vec3(0.8f, 0.8f, 1.f);
				else
					s.color = Vec3(1.f, 0.9f, 0.9f);
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
			pe->op_alpha = POP_LINEAR_SHRINK;
			pe->op_size = POP_LINEAR_SHRINK;
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
			pe->op_alpha = POP_LINEAR_SHRINK;
			pe->op_size = POP_LINEAR_SHRINK;
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
			pe->tex = tWater;
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
			c.pt = Vec2(pos.x, pos.z);
			c.radius = obj->r;
		}
		else if(obj->type == OBJ_HITBOX)
		{
			btTransform& tr = cobj->getWorldTransform();
			m1 = Matrix::RotationY(rot);
			m2 = *obj->matrix * m1;
			Vec3 pos2 = Vec3::TransformZero(m2);
			pos2 += pos;
			tr.setOrigin(ToVector3(pos2));
			tr.setRotation(btQuaternion(rot, 0, 0));

			c.pt = Vec2(pos2.x, pos2.z);
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
			m1 = Matrix::RotationY(rot);
			m2 = Matrix::Translation(pos);
			// skalowanie jakimœ sposobem przechodzi do btWorldTransform i przy rysowaniu jest z³a skala (dwukrotnie u¿yta)
			m3 = Matrix::Scale(1.f / obj->size.x, 1.f, 1.f / obj->size.y);
			m3 = m3 * *obj->matrix * m1 * m2;
			cobj->getWorldTransform().setFromOpenGLMatrix(&m3._11);
			Vec3 out_pos = Vec3::TransformZero(m3);
			Quat q = Quat::CreateFromRotationMatrix(m3);

			float yaw = asin(-2 * (q.x*q.z - q.w*q.y));
			c.pt = Vec2(out_pos.x, out_pos.z);
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
		c.pt = Vec2(pos.x, pos.z);
		c.radius = obj->r*scale;

		btCollisionObject* cobj = new btCollisionObject;
		btCylinderShape* shape = new btCylinderShape(btVector3(obj->r*scale, obj->h*scale, obj->r*scale));
		shapes.push_back(shape);
		cobj->setCollisionShape(shape);
		cobj->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_OBJECT);
		cobj->getWorldTransform().setOrigin(btVector3(pos.x, pos.y + obj->h / 2 * scale, pos.z));
		phy_world->addCollisionObject(cobj, CG_OBJECT);
	}

	if(IsSet(obj->flags, OBJ_CAM_COLLIDERS))
	{
		int roti = (int)round((rot / (PI / 2)));
		for(vector<Mesh::Point>::const_iterator it = obj->mesh->attach_points.begin(), end = obj->mesh->attach_points.end(); it != end; ++it)
		{
			const Mesh::Point& pt = *it;
			if(strncmp(pt.name.c_str(), "camcol", 6) != 0)
				continue;

			m2 = pt.mat * Matrix::RotationY(rot);
			Vec3 pos2 = Vec3::TransformZero(m2) + pos;

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
void Level::ProcessBuildingObjects(LevelArea& area, City* city, InsideBuilding* inside, Mesh* mesh, Mesh* inside_mesh, float rot, int roti,
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
	// x (o - obiekt, r - obrócony obiekt, p - fizyka, s - strefa, c - postaæ, m - maska œwiat³a, d - detal wokó³ obiektu, l - limited rot object)
	// N - wariant (tylko obiekty)
	string token;
	Matrix m1, m2;
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
		if(c == 'o' || c == 'r' || c == 'p' || c == 's' || c == 'c' || c == 'l')
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
		if(roti != 0)
		{
			m2 = pt.mat * Matrix::RotationY(rot);
			pos = Vec3::TransformZero(m2);
		}
		else
			pos = Vec3::TransformZero(pt.mat);
		pos += shift;

		if(c == 'o' || c == 'r' || c == 'l')
		{
			// obiekt / obrócony obiekt
			if(!recreate)
			{
				cstring name;
				int variant = -1;
				if(token[0] == '!')
				{
					// póki co tylko 0-9
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
						terrain->SetH(pos);
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
		}
		else if(c == 'p')
		{
			// fizyka
			if(token == "circle" || token == "circlev")
			{
				bool is_wall = (token == "circle");

				CollisionObject& cobj = Add1(area.tmp->colliders);
				cobj.type = CollisionObject::SPHERE;
				cobj.radius = pt.size.x;
				cobj.pt.x = pos.x;
				cobj.pt.y = pos.z;
				cobj.owner = nullptr;
				cobj.cam_collider = is_wall;

				if(area.area_type == LevelArea::Type::Outside)
				{
					terrain->SetH(pos);
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
				cobj.pt.x = pos.x;
				cobj.pt.y = pos.z;
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
						terrain->SetH(pos);
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

				if(roti != 0)
				{
					cobj.type = CollisionObject::RECTANGLE_ROT;
					cobj.rot = rot;
					cobj.radius = sqrt(cobj.w*cobj.w + cobj.h*cobj.h);
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

				if(roti != 0)
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
				if(roti != 0)
					co->getWorldTransform().setRotation(btQuaternion(rot, 0, 0));

				float w = pt.size.x, h = pt.size.z;
				if(roti == 1 || roti == 3)
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
		}
		else if(c == 's')
		{
			// strefa
			if(!recreate)
			{
				if(token == "enter")
				{
					assert(!inside);

					inside = new InsideBuilding((int)city->inside_buildings.size());
					inside->tmp = TmpLevelArea::Get();
					inside->level_shift = city->inside_offset;
					inside->offset = Vec2(512.f*city->inside_offset.x + 256.f, 512.f*city->inside_offset.y + 256.f);
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
					if(roti == 0 || roti == 2)
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
						o->require_split = true;
						inside->objects.push_back(o);

						ProcessBuildingObjects(*inside, city, inside, inside_mesh, nullptr, 0.f, 0, o->pos, nullptr, nullptr);
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
						terrain->SetH(spawn_point);
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
					door->Register();
					door->pos = pos;
					door->rot = Clip(pt.rot.y + rot);
					door->state = Door::Open;
					door->door2 = (token == "door2");
					door->mesh_inst = new MeshInstance(door->door2 ? game_res->aDoor2 : game_res->aDoor);
					door->mesh_inst->groups[0].speed = 2.f;
					door->phy = new btCollisionObject;
					door->phy->setCollisionShape(shape_door);
					door->phy->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_DOOR);
					door->locked = LOCK_NONE;

					btTransform& tr = door->phy->getWorldTransform();
					Vec3 pos = door->pos;
					pos.y += Door::HEIGHT;
					tr.setOrigin(ToVector3(pos));
					tr.setRotation(btQuaternion(door->rot, 0, 0));
					phy_world->addCollisionObject(door->phy, CG_DOOR);

					if(token != "door") // door2 are closed now, this is intended
					{
						door->state = Door::Closed;
						if(token == "doorl")
							door->locked = 1;
					}
					else
					{
						btVector3& pos = door->phy->getWorldTransform().getOrigin();
						pos.setY(pos.y() - 100.f);
						door->mesh_inst->SetToEnd(door->mesh_inst->mesh->anims[0].name.c_str());
					}

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
					// ten punkt jest u¿ywany w SpawnArenaViewers
				}
				else if(token == "point")
				{
					if(city_building)
					{
						city_building->walk_pt = pos;
						terrain->SetH(city_building->walk_pt);
					}
					else if(out_point)
						*out_point = pos;
				}
				else
					assert(0);
			}
		}
		else if(c == 'c')
		{
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
		}
		else if(c == 'm')
		{
			LightMask& mask = Add1(inside->masks);
			mask.size = Vec2(pt.size.x, pt.size.z);
			mask.pos = Vec2(pos.x, pos.z);
		}
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
				if(roti != 0)
				{
					m1 = Matrix::RotationY(rot);
					m2 = pt.mat * m1;
					pos = Vec3::TransformZero(m2);
				}
				else
					pos = Vec3::TransformZero(pt.mat);
				pos += shift;

				cstring name;
				int variant = -1;
				if(token[0] == '!')
				{
					// póki co tylko 0-9
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
						terrain->SetH(pos);
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
				float rot = obj.rot.y;
				int roti;
				if(Equal(rot, 0))
					roti = 0;
				else if(Equal(rot, PI / 2))
					roti = 1;
				else if(Equal(rot, PI))
					roti = 2;
				else if(Equal(rot, PI * 3 / 2))
					roti = 3;
				else
				{
					assert(0);
					roti = 0;
					rot = 0.f;
				}

				ProcessBuildingObjects(area, nullptr, nullptr, base_obj->mesh, nullptr, rot, roti, obj.pos, nullptr, nullptr, true);
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
			terrain->SetH(pt);

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
			terrain->SetH(pt);

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
			terrain->SetH(pt);

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
void Level::AddGroundItem(LevelArea& area, GroundItem* item)
{
	assert(item);

	if(area.area_type == LevelArea::Type::Outside)
		terrain->SetH(item->pos);
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
GroundItem* Level::SpawnGroundItemInsideAnyRoom(InsideLocationLevel& lvl, const Item* item)
{
	assert(item);
	while(true)
	{
		int id = Rand() % lvl.rooms.size();
		if(!lvl.rooms[id]->IsCorridor())
		{
			GroundItem* item2 = SpawnGroundItemInsideRoom(*lvl.rooms[id], item);
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
		{
			GroundItem* gi = new GroundItem;
			gi->Register();
			gi->count = 1;
			gi->team_count = 1;
			gi->rot = Random(MAX_ANGLE);
			gi->pos = pos;
			gi->item = item;
			local_area->items.push_back(gi);
			return gi;
		}
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
			pt = Vec3(pos.x + b * radius*cos(2 * PI*a / b), 0, pos.y + b * radius*sin(2 * PI*a / b));
		}
		else
		{
			try_exact = false;
			pt = Vec3(pos.x, 0, pos.y);
		}
		global_col.clear();
		GatherCollisionObjects(*local_area, global_col, pt, 0.25f);
		if(!Collide(global_col, pt, 0.25f))
		{
			GroundItem* gi = new GroundItem;
			gi->Register();
			gi->count = 1;
			gi->team_count = 1;
			gi->rot = Random(MAX_ANGLE);
			gi->pos = pt;
			if(local_area->area_type == LevelArea::Type::Outside)
				terrain->SetH(gi->pos);
			gi->item = item;
			local_area->items.push_back(gi);
			return gi;
		}
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
		{
			GroundItem* gi = new GroundItem;
			gi->Register();
			gi->count = 1;
			gi->team_count = 1;
			gi->rot = Random(MAX_ANGLE);
			gi->pos = pt;
			if(local_area->area_type == LevelArea::Type::Outside)
				terrain->SetH(gi->pos);
			gi->item = item;
			local_area->items.push_back(gi);
			return gi;
		}
	}

	return nullptr;
}

//=================================================================================================
void Level::PickableItemAdd(const Item* item)
{
	assert(item);

	for(int i = 0; i < 20; ++i)
	{
		// pobierz punkt
		uint spawn = Rand() % pickable_spawns.size();
		Box& box = pickable_spawns[spawn];
		// ustal pozycjê
		Vec3 pos(Random(box.v1.x, box.v2.x), box.v1.y, Random(box.v1.z, box.v2.z));
		// sprawdŸ kolizjê
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
			gi->rot = Random(MAX_ANGLE);
			float rot = pickable_obj->rot.y,
				s = sin(rot),
				c = cos(rot);
			gi->pos = Vec3(pos.x*c + pos.z*s, pos.y, -pos.x*s + pos.z*c) + pickable_obj->pos;
			pickable_area->items.push_back(gi);

			break;
		}
	}
}

//=================================================================================================
Unit* Level::SpawnUnitInsideRoom(Room &p, UnitData &unit, int level, const Int2& stairs_pt, const Int2& stairs_down_pt)
{
	const float radius = unit.GetRadius();
	Vec3 stairs_pos(2.f*stairs_pt.x + 1.f, 0.f, 2.f*stairs_pt.y + 1.f);

	for(int i = 0; i < 10; ++i)
	{
		Vec3 pt = p.GetRandomPos(radius);

		if(Vec3::Distance(stairs_pos, pt) < ALERT_SPAWN_RANGE)
			continue;

		Int2 my_pt = Int2(int(pt.x / 2), int(pt.y / 2));
		if(my_pt == stairs_down_pt)
			continue;

		global_col.clear();
		GatherCollisionObjects(*local_area, global_col, pt, radius, nullptr);

		if(!Collide(global_col, pt, radius))
		{
			float rot = Random(MAX_ANGLE);
			return game->CreateUnitWithAI(*local_area, unit, level, nullptr, &pt, &rot);
		}
	}

	return nullptr;
}

//=================================================================================================
Unit* Level::SpawnUnitInsideRoomOrNear(InsideLocationLevel& lvl, Room& room, UnitData& ud, int level, const Int2& pt, const Int2& pt2)
{
	Unit* u = SpawnUnitInsideRoom(room, ud, level, pt, pt2);
	if(u)
		return u;

	LocalVector<Room*> connected(room.connected);
	connected.Shuffle();

	for(Room* room : connected)
	{
		u = SpawnUnitInsideRoom(*room, ud, level, pt, pt2);
		if(u)
			return u;
	}

	return nullptr;
}

//=================================================================================================
Unit* Level::SpawnUnitNearLocation(LevelArea& area, const Vec3 &pos, UnitData &unit, const Vec3* look_at, int level, float extra_radius)
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
			return game->CreateUnitWithAI(area, unit, level, nullptr, &tmp_pos, &rot);
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

	return game->CreateUnitWithAI(area, unit, level, nullptr, &pos);
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
		Unit* u = game->CreateUnitWithAI(*inn, ud, level, nullptr, &pos, &rot);
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
	return game->CreateUnitWithAI(area, *spawn.first, spawn.second, nullptr, &pos, &rot);
}

//=================================================================================================
void Level::GatherCollisionObjects(LevelArea& area, vector<CollisionObject>& _objects, const Vec3& _pos, float _radius, const IgnoreObjects* ignore)
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
						CollisionObject& co = Add1(_objects);
						co.pt = Vec2(2.f*x + 1.f, 2.f*z + 1.f);
						co.w = 1.f;
						co.h = 1.f;
						co.type = CollisionObject::RECTANGLE;
					}
				}
			}
		}
		else
		{
			InsideLocation* inside = (InsideLocation*)location;
			InsideLocationLevel& lvl = inside->GetLevelData();
			maxx = min(maxx, lvl.w);
			maxz = min(maxz, lvl.h);

			for(int z = minz; z <= maxz; ++z)
			{
				for(int x = minx; x <= maxx; ++x)
				{
					TILE_TYPE type = lvl.map[x + z * lvl.w].type;
					if(IsBlocking(type))
					{
						CollisionObject& co = Add1(_objects);
						co.pt = Vec2(2.f*x + 1.f, 2.f*z + 1.f);
						co.w = 1.f;
						co.h = 1.f;
						co.type = CollisionObject::RECTANGLE;
					}
					else if(type == STAIRS_DOWN)
					{
						if(!lvl.staircase_down_in_wall)
						{
							CollisionObject& co = Add1(_objects);
							co.pt = Vec2(2.f*x + 1.f, 2.f*z + 1.f);
							co.check = &Level::CollideWithStairs;
							co.check_rect = &Level::CollideWithStairsRect;
							co.extra = 0;
							co.type = CollisionObject::CUSTOM;
						}
					}
					else if(type == STAIRS_UP)
					{
						CollisionObject& co = Add1(_objects);
						co.pt = Vec2(2.f*x + 1.f, 2.f*z + 1.f);
						co.check = &Level::CollideWithStairs;
						co.check_rect = &Level::CollideWithStairsRect;
						co.extra = 1;
						co.type = CollisionObject::CUSTOM;
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
					CollisionObject& co = Add1(_objects);
					co.pt = Vec2(pos.x, pos.z);
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
					CollisionObject& co = Add1(_objects);
					co.pt = Vec2(pos.x, pos.z);
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
					if(CircleToRectangle(_pos.x, _pos.z, _radius, it->pt.x, it->pt.y, it->w, it->h))
						_objects.push_back(*it);
				}
				else
				{
					if(CircleToCircle(_pos.x, _pos.z, _radius, it->pt.x, it->pt.y, it->radius))
						_objects.push_back(*it);
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
							goto ignoruj;
						else if(!*objs)
							break;
						else
							++objs;
					}
					while(1);
				}

				if(it->type == CollisionObject::RECTANGLE)
				{
					if(CircleToRectangle(_pos.x, _pos.z, _radius, it->pt.x, it->pt.y, it->w, it->h))
						_objects.push_back(*it);
				}
				else
				{
					if(CircleToCircle(_pos.x, _pos.z, _radius, it->pt.x, it->pt.y, it->radius))
						_objects.push_back(*it);
				}

			ignoruj:
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
				CollisionObject& co = Add1(_objects);
				co.pt = Vec2((*it)->pos.x, (*it)->pos.z);
				co.type = CollisionObject::RECTANGLE_ROT;
				co.w = Door::WIDTH;
				co.h = Door::THICKNESS;
				co.rot = (*it)->rot;
			}
		}
	}
}

//=================================================================================================
void Level::GatherCollisionObjects(LevelArea& area, vector<CollisionObject>& _objects, const Box2d& _box, const IgnoreObjects* ignore)
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
						CollisionObject& co = Add1(_objects);
						co.pt = Vec2(2.f*x + 1.f, 2.f*z + 1.f);
						co.w = 1.f;
						co.h = 1.f;
						co.type = CollisionObject::RECTANGLE;
					}
				}
			}
		}
		else
		{
			InsideLocation* inside = (InsideLocation*)location;
			InsideLocationLevel& lvl = inside->GetLevelData();
			maxx = min(maxx, lvl.w);
			maxz = min(maxz, lvl.h);

			for(int z = minz; z <= maxz; ++z)
			{
				for(int x = minx; x <= maxx; ++x)
				{
					TILE_TYPE type = lvl.map[x + z * lvl.w].type;
					if(IsBlocking(type))
					{
						CollisionObject& co = Add1(_objects);
						co.pt = Vec2(2.f*x + 1.f, 2.f*z + 1.f);
						co.w = 1.f;
						co.h = 1.f;
						co.type = CollisionObject::RECTANGLE;
					}
					else if(type == STAIRS_DOWN)
					{
						if(!lvl.staircase_down_in_wall)
						{
							CollisionObject& co = Add1(_objects);
							co.pt = Vec2(2.f*x + 1.f, 2.f*z + 1.f);
							co.check = &Level::CollideWithStairs;
							co.check_rect = &Level::CollideWithStairsRect;
							co.extra = 0;
							co.type = CollisionObject::CUSTOM;
						}
					}
					else if(type == STAIRS_UP)
					{
						CollisionObject& co = Add1(_objects);
						co.pt = Vec2(2.f*x + 1.f, 2.f*z + 1.f);
						co.check = &Level::CollideWithStairs;
						co.check_rect = &Level::CollideWithStairsRect;
						co.extra = 1;
						co.type = CollisionObject::CUSTOM;
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
					CollisionObject& co = Add1(_objects);
					co.pt = Vec2((*it)->pos.x, (*it)->pos.z);
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
					CollisionObject& co = Add1(_objects);
					co.pt = Vec2((*it)->pos.x, (*it)->pos.z);
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
					if(RectangleToRectangle(it->pt.x - it->w, it->pt.y - it->h, it->pt.x + it->w, it->pt.y + it->h, _box.v1.x, _box.v1.y, _box.v2.x, _box.v2.y))
						_objects.push_back(*it);
				}
				else
				{
					if(CircleToRectangle(it->pt.x, it->pt.y, it->radius, rectpos.x, rectpos.y, rectsize.x, rectsize.y))
						_objects.push_back(*it);
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
							goto ignoruj;
						else if(!*objs)
							break;
						else
							++objs;
					}
					while(1);
				}

				if(it->type == CollisionObject::RECTANGLE)
				{
					if(RectangleToRectangle(it->pt.x - it->w, it->pt.y - it->h, it->pt.x + it->w, it->pt.y + it->h, _box.v1.x, _box.v1.y, _box.v2.x, _box.v2.y))
						_objects.push_back(*it);
				}
				else
				{
					if(CircleToRectangle(it->pt.x, it->pt.y, it->radius, rectpos.x, rectpos.y, rectsize.x, rectsize.y))
						_objects.push_back(*it);
				}

			ignoruj:
				;
			}
		}
	}
}

//=================================================================================================
bool Level::Collide(const vector<CollisionObject>& _objects, const Vec3& _pos, float _radius)
{
	assert(_radius > 0.f);

	for(vector<CollisionObject>::const_iterator it = _objects.begin(), end = _objects.end(); it != end; ++it)
	{
		switch(it->type)
		{
		case CollisionObject::SPHERE:
			if(Distance(_pos.x, _pos.z, it->pt.x, it->pt.y) <= it->radius + _radius)
				return true;
			break;
		case CollisionObject::RECTANGLE:
			if(CircleToRectangle(_pos.x, _pos.z, _radius, it->pt.x, it->pt.y, it->w, it->h))
				return true;
			break;
		case CollisionObject::RECTANGLE_ROT:
			if(CircleToRotatedRectangle(_pos.x, _pos.z, _radius, it->pt.x, it->pt.y, it->w, it->h, it->rot))
				return true;
			break;
		case CollisionObject::CUSTOM:
			if((this->*(it->check))(*it, _pos, _radius))
				return true;
			break;
		}
	}

	return false;
}

//=================================================================================================
bool Level::Collide(const vector<CollisionObject>& _objects, const Box2d& _box, float _margin)
{
	Box2d box = _box;
	box.v1.x -= _margin;
	box.v1.y -= _margin;
	box.v2.x += _margin;
	box.v2.y += _margin;

	Vec2 rectpos = _box.Midpoint(),
		rectsize = _box.Size() / 2;

	for(vector<CollisionObject>::const_iterator it = _objects.begin(), end = _objects.end(); it != end; ++it)
	{
		switch(it->type)
		{
		case CollisionObject::SPHERE:
			if(CircleToRectangle(it->pt.x, it->pt.y, it->radius + _margin, rectpos.x, rectpos.y, rectsize.x, rectsize.y))
				return true;
			break;
		case CollisionObject::RECTANGLE:
			if(RectangleToRectangle(box.v1.x, box.v1.y, box.v2.x, box.v2.y, it->pt.x - it->w, it->pt.y - it->h, it->pt.x + it->w, it->pt.y + it->h))
				return true;
			break;
		case CollisionObject::RECTANGLE_ROT:
			{
				RotRect r1, r2;
				r1.center = it->pt;
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
bool Level::Collide(const vector<CollisionObject>& _objects, const Box2d& _box, float margin, float _rot)
{
	if(!NotZero(_rot))
		return Collide(_objects, _box, margin);

	Box2d box = _box;
	box.v1.x -= margin;
	box.v1.y -= margin;
	box.v2.x += margin;
	box.v2.y += margin;

	Vec2 rectpos = box.Midpoint(),
		rectsize = box.Size() / 2;

	for(vector<CollisionObject>::const_iterator it = _objects.begin(), end = _objects.end(); it != end; ++it)
	{
		switch(it->type)
		{
		case CollisionObject::SPHERE:
			if(CircleToRotatedRectangle(it->pt.x, it->pt.y, it->radius, rectpos.x, rectpos.y, rectsize.x, rectsize.y, _rot))
				return true;
			break;
		case CollisionObject::RECTANGLE:
			{
				RotRect r1, r2;
				r1.center = it->pt;
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
				r1.center = it->pt;
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
bool Level::CollideWithStairs(const CollisionObject& _co, const Vec3& _pos, float _radius) const
{
	assert(_co.type == CollisionObject::CUSTOM && _co.check == &Level::CollideWithStairs && !location->outside && _radius > 0.f);

	InsideLocation* inside = (InsideLocation*)location;
	InsideLocationLevel& lvl = inside->GetLevelData();

	GameDirection dir;
	if(_co.extra == 0)
	{
		assert(!lvl.staircase_down_in_wall);
		dir = lvl.staircase_down_dir;
	}
	else
		dir = lvl.staircase_up_dir;

	if(dir != GDIR_DOWN)
	{
		if(CircleToRectangle(_pos.x, _pos.z, _radius, _co.pt.x, _co.pt.y - 0.95f, 1.f, 0.05f))
			return true;
	}

	if(dir != GDIR_LEFT)
	{
		if(CircleToRectangle(_pos.x, _pos.z, _radius, _co.pt.x - 0.95f, _co.pt.y, 0.05f, 1.f))
			return true;
	}

	if(dir != GDIR_UP)
	{
		if(CircleToRectangle(_pos.x, _pos.z, _radius, _co.pt.x, _co.pt.y + 0.95f, 1.f, 0.05f))
			return true;
	}

	if(dir != GDIR_RIGHT)
	{
		if(CircleToRectangle(_pos.x, _pos.z, _radius, _co.pt.x + 0.95f, _co.pt.y, 0.05f, 1.f))
			return true;
	}

	return false;
}

//=================================================================================================
bool Level::CollideWithStairsRect(const CollisionObject& _co, const Box2d& _box) const
{
	assert(_co.type == CollisionObject::CUSTOM && _co.check_rect == &Level::CollideWithStairsRect && !location->outside);

	InsideLocation* inside = (InsideLocation*)location;
	InsideLocationLevel& lvl = inside->GetLevelData();

	GameDirection dir;
	if(_co.extra == 0)
	{
		assert(!lvl.staircase_down_in_wall);
		dir = lvl.staircase_down_dir;
	}
	else
		dir = lvl.staircase_up_dir;

	if(dir != GDIR_DOWN)
	{
		if(RectangleToRectangle(_box.v1.x, _box.v1.y, _box.v2.x, _box.v2.y, _co.pt.x - 1.f, _co.pt.y - 1.f, _co.pt.x + 1.f, _co.pt.y - 0.9f))
			return true;
	}

	if(dir != GDIR_LEFT)
	{
		if(RectangleToRectangle(_box.v1.x, _box.v1.y, _box.v2.x, _box.v2.y, _co.pt.x - 1.f, _co.pt.y - 1.f, _co.pt.x - 0.9f, _co.pt.y + 1.f))
			return true;
	}

	if(dir != GDIR_UP)
	{
		if(RectangleToRectangle(_box.v1.x, _box.v1.y, _box.v2.x, _box.v2.y, _co.pt.x - 1.f, _co.pt.y + 0.9f, _co.pt.x + 1.f, _co.pt.y + 1.f))
			return true;
	}

	if(dir != GDIR_RIGHT)
	{
		if(RectangleToRectangle(_box.v1.x, _box.v1.y, _box.v2.x, _box.v2.y, _co.pt.x + 0.9f, _co.pt.y - 1.f, _co.pt.x + 1.f, _co.pt.y + 1.f))
			return true;
	}

	return false;
}

//=================================================================================================
void Level::CreateBlood(LevelArea& area, const Unit& u, bool fully_created)
{
	if(!game_res->tBloodSplat[u.data->blood] || IsSet(u.data->flags2, F2_BLOODLESS))
		return;

	Blood& b = Add1(area.bloods);
	if(u.human_data)
		u.mesh_inst->SetupBones(&u.human_data->mat_scale[0]);
	else
		u.mesh_inst->SetupBones();
	b.pos = u.GetLootCenter();
	b.type = u.data->blood;
	b.rot = Random(MAX_ANGLE);
	b.size = (fully_created ? 1.f : 0.f);
	b.scale = u.data->blood_size;

	if(area.have_terrain)
	{
		b.pos.y = terrain->GetH(b.pos) + 0.05f;
		terrain->GetAngle(b.pos.x, b.pos.z, b.normal);
	}
	else
	{
		b.pos.y = u.pos.y + 0.05f;
		b.normal = Vec3(0, 1, 0);
	}
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
		terrain->SetH(unit.pos);

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
	trap.pos = Vec3(2.f*pt.x + Random(trap.base->rw, 2.f - trap.base->rw), 0.f, 2.f*pt.y + Random(trap.base->h, 2.f - trap.base->h));
	trap.obj.base = nullptr;
	trap.obj.mesh = trap.base->mesh;
	trap.obj.pos = trap.pos;
	trap.obj.scale = 1.f;

	if(type == TRAP_ARROW || type == TRAP_POISON)
	{
		trap.obj.rot = Vec3(0, 0, 0);

		static vector<TrapLocation> possible;

		InsideLocationLevel& lvl = ((InsideLocation*)location)->GetLevelData();

		// ustal tile i dir
		for(int i = 0; i < 4; ++i)
		{
			GameDirection dir = (GameDirection)i;
			bool ok = false;
			int j;

			for(j = 1; j <= 10; ++j)
			{
				if(IsBlocking(lvl.map[pt.x + DirToPos(dir).x*j + (pt.y + DirToPos(dir).y*j)*lvl.w]))
				{
					if(j != 1)
						ok = true;
					break;
				}
			}

			if(ok)
			{
				trap.tile = pt + DirToPos(dir) * j;

				if(CanShootAtLocation(Vec3(trap.pos.x + (2.f*j - 1.2f)*DirToPos(dir).x, 1.f, trap.pos.z + (2.f*j - 1.2f)*DirToPos(dir).y),
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
	else
	{
		trap.obj.rot = Vec3(0, PI / 2 * (Rand() % 4), 0);
		trap.obj.base = &BaseObject::obj_alpha;
	}

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
	// up³yw czasu
	// 1-10 dni (usuñ zw³oki)
	// 5-30 dni (usuñ krew)
	// 1+ zamknij/otwórz drzwi
	for(LevelArea& area : ForEachArea())
	{
		if(days <= 10)
		{
			// usuñ niektóre zw³oki i przedmioty
			for(Unit*& u : area.units)
			{
				if(!u->IsAlive() && Random(4, 10) < days)
				{
					delete u;
					u = nullptr;
				}
			}
			RemoveNullElements(area.units);
			auto from = std::remove_if(area.items.begin(), area.items.end(), RemoveRandomPred<GroundItem*>(days, 0, 10));
			auto end = area.items.end();
			if(from != end)
			{
				for(vector<GroundItem*>::iterator it = from; it != end; ++it)
					delete *it;
				area.items.erase(from, end);
			}
		}
		else
		{
			// usuñ wszystkie zw³oki i przedmioty
			if(reset)
				DeleteElements(area.units);
			else
				DeleteElements(area.units, [](Unit* unit) { return !unit->IsAlive(); });
			DeleteElements(area.items);
		}

		// wylecz jednostki
		if(!reset)
		{
			for(vector<Unit*>::iterator it = area.units.begin(), end = area.units.end(); it != end; ++it)
			{
				if((*it)->IsAlive())
					(*it)->NaturalHealing(days);
			}
		}

		if(days > 30)
		{
			// usuñ krew
			area.bloods.clear();
		}
		else if(days >= 5)
		{
			// usuñ czêœciowo krew
			RemoveElements(area.bloods, RemoveRandomPred<Blood>(days, 4, 30));
		}

		if(days > 30)
		{
			// usuñ wszystkie jednorazowe pu³apki
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
			// usuñ czêœæ 1razowych pu³apek
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

		// losowo otwórz/zamknij drzwi
		for(vector<Door*>::iterator it = area.doors.begin(), end = area.doors.end(); it != end; ++it)
		{
			Door& door = **it;
			if(door.locked == 0)
			{
				if(Rand() % 100 < open_chance)
					door.state = Door::Open;
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
void Level::OnReenterLevel()
{
	for(LevelArea& area : ForEachArea())
	{
		// odtwórz skrzynie
		for(vector<Chest*>::iterator it = area.chests.begin(), end = area.chests.end(); it != end; ++it)
		{
			Chest& chest = **it;

			chest.mesh_inst = new MeshInstance(game_res->aChest);
		}

		// odtwórz drzwi
		for(vector<Door*>::iterator it = area.doors.begin(), end = area.doors.end(); it != end; ++it)
		{
			Door& door = **it;

			// animowany model
			door.mesh_inst = new MeshInstance(door.door2 ? game_res->aDoor2 : game_res->aDoor);
			door.mesh_inst->groups[0].speed = 2.f;

			// fizyka
			door.phy = new btCollisionObject;
			door.phy->setCollisionShape(shape_door);
			door.phy->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_DOOR);
			btTransform& tr = door.phy->getWorldTransform();
			Vec3 pos = door.pos;
			pos.y += Door::HEIGHT;
			tr.setOrigin(ToVector3(pos));
			tr.setRotation(btQuaternion(door.rot, 0, 0));
			phy_world->addCollisionObject(door.phy, CG_DOOR);

			// czy otwarte
			if(door.state == Door::Open)
			{
				btVector3& pos = door.phy->getWorldTransform().getOrigin();
				pos.setY(pos.y() - 100.f);
				door.mesh_inst->SetToEnd(door.mesh_inst->mesh->anims[0].name.c_str());
			}
		}
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
	if(city_ctx)
		return;

	bool is_clear = true;
	for(vector<Unit*>::iterator it = local_area->units.begin(), end = local_area->units.end(); it != end; ++it)
	{
		if((*it)->IsAlive() && game->pc->unit->IsEnemy(**it, true))
		{
			is_clear = false;
			break;
		}
	}

	if(is_clear)
	{
		bool cleared = false;
		if(!location->outside)
		{
			InsideLocation* inside = static_cast<InsideLocation*>(location);
			if(inside->IsMultilevel())
			{
				if(static_cast<MultiInsideLocation*>(inside)->LevelCleared())
					cleared = true;
			}
			else
				cleared = true;
		}
		else
			cleared = true;

		if(cleared && location->state != LS_HIDDEN)
			location->state = LS_CLEARED;

		bool prevent = false;
		if(event_handler)
			prevent = event_handler->HandleLocationEvent(LocationEventHandler::CLEARED);

		if(cleared && prevent && !location->group->IsEmpty())
		{
			if(location->type == L_CAMP)
				world->AddNews(Format(txNewsCampCleared, world->GetLocation(world->GetNearestSettlement(location->pos))->name.c_str()));
			else
				world->AddNews(Format(txNewsLocCleared, location->name.c_str()));
		}
	}
}

//=================================================================================================
// usuwa podany przedmiot ze œwiata
// u¿ywane w queœcie z kamieniem szaleñców
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
	assert(!location->outside);

	InsideLocation* inside = (InsideLocation*)location;
	InsideLocationLevel& lvl = inside->GetLevelData();
	Tile* m = lvl.map;
	int w = lvl.w,
		h = lvl.h;

	for(int y = 1; y < h - 1; ++y)
	{
		for(int x = 1; x < w - 1; ++x)
		{
			if(IsBlocking(m[x + y * w]) && (!IsBlocking(m[x - 1 + (y - 1)*w]) || !IsBlocking(m[x + (y - 1)*w]) || !IsBlocking(m[x + 1 + (y - 1)*w])
				|| !IsBlocking(m[x - 1 + y * w]) || !IsBlocking(m[x + 1 + y * w])
				|| !IsBlocking(m[x - 1 + (y + 1)*w]) || !IsBlocking(m[x + (y + 1)*w]) || !IsBlocking(m[x + 1 + (y + 1)*w])))
			{
				SpawnDungeonCollider(Vec3(2.f*x + 1.f, 2.f, 2.f*y + 1.f));
			}
		}
	}

	// left/right wall
	for(int i = 1; i < h - 1; ++i)
	{
		// left
		if(IsBlocking(m[i*w]) && !IsBlocking(m[1 + i * w]))
			SpawnDungeonCollider(Vec3(1.f, 2.f, 2.f*i + 1.f));

		// right
		if(IsBlocking(m[i*w + w - 1]) && !IsBlocking(m[w - 2 + i * w]))
			SpawnDungeonCollider(Vec3(2.f*(w - 1) + 1.f, 2.f, 2.f*i + 1.f));
	}

	// front/back wall
	for(int i = 1; i < lvl.w - 1; ++i)
	{
		// front
		if(IsBlocking(m[i + (h - 1)*w]) && !IsBlocking(m[i + (h - 2)*w]))
			SpawnDungeonCollider(Vec3(2.f*i + 1.f, 2.f, 2.f*(h - 1) + 1.f));

		// back
		if(IsBlocking(m[i]) && !IsBlocking(m[i + w]))
			SpawnDungeonCollider(Vec3(2.f*i + 1.f, 2.f, 1.f));
	}

	// up stairs
	if(inside->HaveUpStairs())
	{
		btCollisionObject* cobj = new btCollisionObject;
		cobj->setCollisionShape(shape_stairs);
		cobj->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_BUILDING);
		cobj->getWorldTransform().setOrigin(btVector3(2.f*lvl.staircase_up.x + 1.f, 0.f, 2.f*lvl.staircase_up.y + 1.f));
		cobj->getWorldTransform().setRotation(btQuaternion(DirToRot(lvl.staircase_up_dir), 0, 0));
		phy_world->addCollisionObject(cobj, CG_BUILDING);
	}

	// room floors/ceilings
	dungeon_shape_pos.clear();
	dungeon_shape_index.clear();
	int index = 0;

	if((inside->type == L_DUNGEON && inside->target == LABYRINTH) || inside->type == L_CAVE)
	{
		const float h = Room::HEIGHT;
		for(int x = 0; x < 16; ++x)
		{
			for(int y = 0; y < 16; ++y)
			{
				// floor
				dungeon_shape_pos.push_back(Vec3(2.f * x * lvl.w / 16, 0, 2.f * y * lvl.h / 16));
				dungeon_shape_pos.push_back(Vec3(2.f * (x + 1) * lvl.w / 16, 0, 2.f * y * lvl.h / 16));
				dungeon_shape_pos.push_back(Vec3(2.f * x * lvl.w / 16, 0, 2.f * (y + 1) * lvl.h / 16));
				dungeon_shape_pos.push_back(Vec3(2.f * (x + 1) * lvl.w / 16, 0, 2.f * (y + 1) * lvl.h / 16));
				dungeon_shape_index.push_back(index);
				dungeon_shape_index.push_back(index + 1);
				dungeon_shape_index.push_back(index + 2);
				dungeon_shape_index.push_back(index + 2);
				dungeon_shape_index.push_back(index + 1);
				dungeon_shape_index.push_back(index + 3);
				index += 4;

				// ceil
				dungeon_shape_pos.push_back(Vec3(2.f * x * lvl.w / 16, h, 2.f * y * lvl.h / 16));
				dungeon_shape_pos.push_back(Vec3(2.f * (x + 1) * lvl.w / 16, h, 2.f * y * lvl.h / 16));
				dungeon_shape_pos.push_back(Vec3(2.f * x * lvl.w / 16, h, 2.f * (y + 1) * lvl.h / 16));
				dungeon_shape_pos.push_back(Vec3(2.f * (x + 1) * lvl.w / 16, h, 2.f * (y + 1) * lvl.h / 16));
				dungeon_shape_index.push_back(index);
				dungeon_shape_index.push_back(index + 2);
				dungeon_shape_index.push_back(index + 1);
				dungeon_shape_index.push_back(index + 2);
				dungeon_shape_index.push_back(index + 3);
				dungeon_shape_index.push_back(index + 1);
				index += 4;
			}
		}
	}

	for(Room* room : lvl.rooms)
	{
		// floor
		dungeon_shape_pos.push_back(Vec3(2.f * room->pos.x, 0, 2.f * room->pos.y));
		dungeon_shape_pos.push_back(Vec3(2.f * (room->pos.x + room->size.x), 0, 2.f * room->pos.y));
		dungeon_shape_pos.push_back(Vec3(2.f * room->pos.x, 0, 2.f * (room->pos.y + room->size.y)));
		dungeon_shape_pos.push_back(Vec3(2.f * (room->pos.x + room->size.x), 0, 2.f * (room->pos.y + room->size.y)));
		dungeon_shape_index.push_back(index);
		dungeon_shape_index.push_back(index + 1);
		dungeon_shape_index.push_back(index + 2);
		dungeon_shape_index.push_back(index + 2);
		dungeon_shape_index.push_back(index + 1);
		dungeon_shape_index.push_back(index + 3);
		index += 4;

		// ceil
		const float h = (room->IsCorridor() ? Room::HEIGHT_LOW : Room::HEIGHT);
		dungeon_shape_pos.push_back(Vec3(2.f * room->pos.x, h, 2.f * room->pos.y));
		dungeon_shape_pos.push_back(Vec3(2.f * (room->pos.x + room->size.x), h, 2.f * room->pos.y));
		dungeon_shape_pos.push_back(Vec3(2.f * room->pos.x, h, 2.f * (room->pos.y + room->size.y)));
		dungeon_shape_pos.push_back(Vec3(2.f * (room->pos.x + room->size.x), h, 2.f * (room->pos.y + room->size.y)));
		dungeon_shape_index.push_back(index);
		dungeon_shape_index.push_back(index + 2);
		dungeon_shape_index.push_back(index + 1);
		dungeon_shape_index.push_back(index + 2);
		dungeon_shape_index.push_back(index + 3);
		dungeon_shape_index.push_back(index + 1);
		index += 4;
	}

	delete dungeon_shape;
	delete dungeon_shape_data;

	dungeon_shape_data = new btTriangleIndexVertexArray(dungeon_shape_index.size() / 3, dungeon_shape_index.data(), sizeof(int) * 3,
		dungeon_shape_pos.size(), (btScalar*)dungeon_shape_pos.data(), sizeof(Vec3));
	dungeon_shape = new btBvhTriangleMeshShape(dungeon_shape_data, true);

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
	InsideLocation* inside = (InsideLocation*)location;
	InsideLocationLevel& lvl = inside->GetLevelData();

	if(enter_from >= ENTER_FROM_PORTAL)
		return PosToPt(inside->GetPortal(enter_from)->GetSpawnPos());
	else if(enter_from == ENTER_FROM_DOWN_LEVEL)
		return lvl.GetDownStairsFrontTile();
	else
		return lvl.GetUpStairsFrontTile();
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

		// lewa krawêdŸ
		best_dist = abs(pos.x - 32.f);

		// prawa krawêdŸ
		dist = abs(pos.x - (256.f - 32.f));
		if(dist < best_dist)
		{
			best_dist = dist;
			best = 1;
		}

		// górna krawêdŸ
		dist = abs(pos.z - 32.f);
		if(dist < best_dist)
		{
			best_dist = dist;
			best = 2;
		}

		// dolna krawêdŸ
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
		Int2& pt = inside->GetLevelData().staircase_up;
		return Vec3(2.f*pt.x + 1, 0, 2.f*pt.y + 1);
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
					&& LineToRectangle(u1.pos, u2.pos, Vec2(2.f*x, 2.f*y), Vec2(2.f*(x + 1), 2.f*(y + 1))))
					return false;
			}
		}
	}
	else if(area.area_type == LevelArea::Type::Inside)
	{
		InsideLocation* inside = (InsideLocation*)location;
		InsideLocationLevel& lvl = inside->GetLevelData();

		int xmin = max(0, min(tile1.x, tile2.x)),
			xmax = min(lvl.w, max(tile1.x, tile2.x)),
			ymin = max(0, min(tile1.y, tile2.y)),
			ymax = min(lvl.h, max(tile1.y, tile2.y));

		for(int y = ymin; y <= ymax; ++y)
		{
			for(int x = xmin; x <= xmax; ++x)
			{
				if(IsBlocking(lvl.map[x + y * lvl.w].type) && LineToRectangle(u1.pos, u2.pos, Vec2(2.f*x, 2.f*y), Vec2(2.f*(x + 1), 2.f*(y + 1))))
					return false;
				if(lvl.map[x + y * lvl.w].type == DOORS)
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

			Box2d box(it->pt.x - it->w, it->pt.y - it->h, it->pt.x + it->w, it->pt.y + it->h);
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
				if(outside->tiles[x + y * OutsideLocation::size].IsBlocking() && LineToRectangle(v1, v2, Vec2(2.f*x, 2.f*y), Vec2(2.f*(x + 1), 2.f*(y + 1))))
					return false;
			}
		}
	}
	else if(area.area_type == LevelArea::Type::Inside)
	{
		InsideLocation* inside = static_cast<InsideLocation*>(location);
		InsideLocationLevel& lvl = inside->GetLevelData();

		int xmin = max(0, min(tile1.x, tile2.x)),
			xmax = min(lvl.w, max(tile1.x, tile2.x)),
			ymin = max(0, min(tile1.y, tile2.y)),
			ymax = min(lvl.h, max(tile1.y, tile2.y));

		for(int y = ymin; y <= ymax; ++y)
		{
			for(int x = xmin; x <= xmax; ++x)
			{
				if(IsBlocking(lvl.map[x + y * lvl.w].type) && LineToRectangle(v1, v2, Vec2(2.f*x, 2.f*y), Vec2(2.f*(x + 1), 2.f*(y + 1))))
					return false;
				if(lvl.map[x + y * lvl.w].type == DOORS)
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

			Box2d box(it->pt.x - it->w, it->pt.y - it->h, it->pt.x + it->w, it->pt.y + it->h);
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
					game->GiveDmg(*u, u->hp);
			}
		}
		break;
	case 1: // kill all except player/ignore
		for(LevelArea& area : ForEachArea())
		{
			for(Unit* u : area.units)
			{
				if(u->IsAlive() && !u->IsPlayer() && u != ignore)
					game->GiveDmg(*u, u->hp);
			}
		}
		break;
	}

	return true;
}

//=================================================================================================
void Level::AddPlayerTeam(const Vec3& pos, float rot, bool reenter, bool hide_weapon)
{
	for(Unit& unit : team->members)
	{
		if(!reenter)
		{
			local_area->units.push_back(&unit);
			unit.CreatePhysics();
			if(unit.IsHero())
				game->ais.push_back(unit.ai);
		}

		if(hide_weapon || unit.weapon_state == WS_HIDING)
		{
			unit.weapon_state = WS_HIDDEN;
			unit.weapon_taken = W_NONE;
			unit.weapon_hiding = W_NONE;
			if(unit.action == A_TAKE_WEAPON)
				unit.action = A_NONE;
		}
		else if(unit.weapon_state == WS_TAKING)
			unit.weapon_state = WS_TAKEN;

		unit.rot = rot;
		unit.animation = unit.current_animation = ANI_STAND;
		unit.mesh_inst->Play(NAMES::ani_stand, PLAY_PRIO1, 0);
		unit.BreakAction();
		unit.SetAnimationAtEnd();
		if(unit.area && unit.area->area_type == LevelArea::Type::Building && reenter)
		{
			RemoveElement(unit.area->units, &unit);
			local_area->units.push_back(&unit);
		}
		unit.area = local_area;

		if(unit.IsAI())
		{
			unit.ai->state = AIController::Idle;
			unit.ai->idle_action = AIController::Idle_None;
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

	TextureLock lock(game->tMinimap.tex);
	InsideLocationLevel& lvl = ((InsideLocation*)location)->GetLevelData();

	for(vector<Int2>::iterator it = minimap_reveal.begin(), end = minimap_reveal.end(); it != end; ++it)
	{
		Tile& p = lvl.map[it->x + (lvl.w - it->y - 1)*lvl.w];
		SetBit(p.flags, Tile::F_REVEALED);
		uint* pix = lock[it->y] + it->x;
		if(OR2_EQ(p.type, WALL, BLOCKADE_WALL))
			*pix = Color(100, 100, 100);
		else if(p.type == DOORS)
			*pix = Color(127, 51, 0);
		else
			*pix = Color(220, 220, 240);
	}

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

	InsideLocationLevel& lvl = static_cast<InsideLocation*>(location)->GetLevelData();

	for(int y = 0; y < lvl.h; ++y)
	{
		for(int x = 0; x < lvl.w; ++x)
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
	f << reenter;
	location->Write(f);
	f.WriteCasted<byte>(GetLocationMusic());

	if(!net->mp_load && !reenter)
		return;

	for(LevelArea& area : ForEachArea())
	{
		TmpLevelArea& tmp_area = *area.tmp;

		// bullets
		f.Write(tmp_area.bullets.size());
		for(Bullet& bullet : tmp_area.bullets)
		{
			f << bullet.pos;
			f << bullet.rot;
			f << bullet.speed;
			f << bullet.yspeed;
			f << bullet.timer;
			f << (bullet.owner ? bullet.owner->id : -1);
			if(bullet.spell)
				f << bullet.spell->id;
			else
				f.Write0();
		}

		// explosions
		f.Write(tmp_area.explos.size());
		for(Explo* explo : tmp_area.explos)
			explo->Write(f);

		// electros
		f.Write(tmp_area.electros.size());
		for(Electro* electro : tmp_area.electros)
			electro->Write(f);
	}
}

//=================================================================================================
bool Level::Read(BitStreamReader& f, bool loaded_resources)
{
	// location
	f >> reenter;
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
	if(world->IsBossLevel())
		game->SetMusic();
	else
		game->SetMusic(music);

	if(!net->mp_load && !reenter)
		return true;

	for(LevelArea& area : ForEachArea())
	{
		TmpLevelArea& tmp_area = *area.tmp;

		// bullets
		uint count;
		f >> count;
		if(!f.Ensure(count * Bullet::MIN_SIZE))
		{
			Error("Read level: Broken bullet count.");
			return false;
		}
		tmp_area.bullets.resize(count);
		for(Bullet& bullet : tmp_area.bullets)
		{
			f >> bullet.pos;
			f >> bullet.rot;
			f >> bullet.speed;
			f >> bullet.yspeed;
			f >> bullet.timer;
			int unit_id = f.Read<int>();
			const string& spell_id = f.ReadString1();
			if(!f)
			{
				Error("Read level: Broken bullet.");
				return false;
			}
			if(spell_id.empty())
			{
				bullet.spell = nullptr;
				bullet.mesh = game_res->aArrow;
				bullet.pe = nullptr;
				bullet.remove = false;
				bullet.tex = nullptr;
				bullet.tex_size = 0.f;

				TrailParticleEmitter* tpe = new TrailParticleEmitter;
				tpe->fade = 0.3f;
				tpe->color1 = Vec4(1, 1, 1, 0.5f);
				tpe->color2 = Vec4(1, 1, 1, 0);
				tpe->Init(50);
				tmp_area.tpes.push_back(tpe);
				bullet.trail = tpe;

				TrailParticleEmitter* tpe2 = new TrailParticleEmitter;
				tpe2->fade = 0.3f;
				tpe2->color1 = Vec4(1, 1, 1, 0.5f);
				tpe2->color2 = Vec4(1, 1, 1, 0);
				tpe2->Init(50);
				tmp_area.tpes.push_back(tpe2);
				bullet.trail2 = tpe2;
			}
			else
			{
				Spell* spell_ptr = Spell::TryGet(spell_id);
				if(!spell_ptr)
				{
					Error("Read level: Missing spell '%s'.", spell_id.c_str());
					return false;
				}

				Spell& spell = *spell_ptr;
				bullet.spell = &spell;
				bullet.mesh = spell.mesh;
				bullet.tex = spell.tex;
				bullet.tex_size = spell.size;
				bullet.remove = false;
				bullet.trail = nullptr;
				bullet.trail2 = nullptr;
				bullet.pe = nullptr;

				if(spell.tex_particle)
				{
					ParticleEmitter* pe = new ParticleEmitter;
					pe->tex = spell.tex_particle;
					pe->emision_interval = 0.1f;
					pe->life = -1;
					pe->particle_life = 0.5f;
					pe->emisions = -1;
					pe->spawn_min = 3;
					pe->spawn_max = 4;
					pe->max_particles = 50;
					pe->pos = bullet.pos;
					pe->speed_min = Vec3(-1, -1, -1);
					pe->speed_max = Vec3(1, 1, 1);
					pe->pos_min = Vec3(-spell.size, -spell.size, -spell.size);
					pe->pos_max = Vec3(spell.size, spell.size, spell.size);
					pe->size = spell.size_particle;
					pe->op_size = POP_LINEAR_SHRINK;
					pe->alpha = 1.f;
					pe->op_alpha = POP_LINEAR_SHRINK;
					pe->mode = 1;
					pe->Init();
					tmp_area.pes.push_back(pe);
					bullet.pe = pe;
				}
			}

			if(unit_id != -1)
			{
				bullet.owner = FindUnit(unit_id);
				if(!bullet.owner)
				{
					Error("Read level: Missing bullet owner %d.", unit_id);
					return false;
				}
			}
			else
				bullet.owner = nullptr;
		}

		// explosions
		f >> count;
		if(!f.Ensure(count * Explo::MIN_SIZE))
		{
			Error("Read level: Broken explosion count.");
			return false;
		}
		tmp_area.explos.resize(count);
		for(Explo*& explo : tmp_area.explos)
		{
			explo = new Explo;
			if(!explo->Read(f))
			{
				Error("Read level: Broken explosion.");
				return false;
			}
		}

		// electro effects
		f >> count;
		if(!f.Ensure(count * Electro::MIN_SIZE))
		{
			Error("Read level: Broken electro count.");
			return false;
		}
		tmp_area.electros.resize(count);
		for(Electro*& electro : tmp_area.electros)
		{
			electro = new Electro;
			if(!electro->Read(f))
			{
				Error("Read level: Broken electro.");
				return false;
			}
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
			area.bloods.clear();
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
Vec4 Level::GetFogParams()
{
	if(cl_fog)
		return fog_params;
	else
		return Vec4(camera.draw_range, camera.draw_range + 1, 1, 0);
}

//=================================================================================================
Vec4 Level::GetAmbientColor()
{
	if(!cl_lighting)
		return Vec4(1, 1, 1, 1);
	return ambient_color;
}

//=================================================================================================
Vec4 Level::GetLightDir()
{
	Vec3 light_dir(sin(light_angle), 2.f, cos(light_angle));
	light_dir.Normalize();
	return Vec4(light_dir, 1);
}

//=================================================================================================
void Level::SetOutsideParams()
{
	camera.draw_range = 80.f;
	game->clear_color_next = Color::White;
	fog_params = Vec4(40, 80, 40, 0);
	fog_color = Vec4(0.9f, 0.85f, 0.8f, 1);
	ambient_color = Vec4(0.5f, 0.5f, 0.5f, 1);
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
	return callback.clear;
}

//=================================================================================================
bool Level::RayTest(const Vec3& from, const Vec3& to, Unit* ignore, Vec3& hitpoint, Unit*& hitted)
{
	RaytestClosestUnitCallback callback(ignore);
	phy_world->rayTest(ToVector3(from), ToVector3(to), callback);

	if(callback.hitted)
	{
		hitpoint = from + (to - from) * callback.fraction;
		hitted = callback.hitted;
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
	//t_from.getBasis().setRotation(ToQuaternion(Quat::CreateFromYawPitchRoll(rot, 0, 0)));
	t_to.setIdentity();
	t_to.setOrigin(ToVector3(dir) + t_from.getOrigin());
	//t_to.setBasis(t_from.getBasis());

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

	// idŸ prosto po x i z
	if(!Collide(global_col, new_pos, radius))
	{
		if(is_small)
			*is_small = (Vec3::Distance(pos, new_pos) < SMALL_DISTANCE);
		pos = new_pos;
		return 3;
	}

	// idŸ po x
	Vec3 new_pos2 = me->pos;
	new_pos2.x = new_pos.x;
	if(!Collide(global_col, new_pos2, radius))
	{
		if(is_small)
			*is_small = (Vec3::Distance(pos, new_pos2) < SMALL_DISTANCE);
		pos = new_pos2;
		return 1;
	}

	// idŸ po z
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
	pe->op_size = POP_LINEAR_SHRINK;
	pe->alpha = 0.5f;
	pe->op_alpha = POP_LINEAR_SHRINK;
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
