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

Level L;

//=================================================================================================
Level::Level() : terrain(nullptr), terrain_shape(nullptr), dungeon_shape(nullptr), dungeon_shape_data(nullptr), shape_wall(nullptr), shape_stairs(nullptr),
shape_stairs_part(), shape_block(nullptr), shape_barrier(nullptr), shape_door(nullptr), shape_arrow(nullptr), shape_summon(nullptr)
{
}

//=================================================================================================
void Level::LoadLanguage()
{
	txLocationText = Str("locationText");
	txLocationTextMap = Str("locationTextMap");
	txNewsCampCleared = Str("newsCampCleared");
	txNewsLocCleared = Str("newsLocCleared");
}

//=================================================================================================
void Level::LoadData()
{
	auto& tex_mgr = ResourceManager::Get<Texture>();
	tFlare = tex_mgr.AddLoadTask("flare.png");
	tFlare2 = tex_mgr.AddLoadTask("flare2.png");
	tWater = tex_mgr.AddLoadTask("water.png");
}

//=================================================================================================
void Level::PostInit()
{
	Game& game = Game::Get();

	phy_world = game.phy_world;

	terrain = new Terrain;
	TerrainOptions terrain_options;
	terrain_options.n_parts = 8;
	terrain_options.tex_size = 256;
	terrain_options.tile_size = 2.f;
	terrain_options.tiles_per_part = 16;
	terrain->Init(game.device, terrain_options);
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
	shape_door = new btBoxShape(btVector3(0.842f, 1.319f, 0.181f));
	shape_block = new btBoxShape(btVector3(1.f, 4.f, 1.f));
	shape_barrier = new btBoxShape(btVector3(size / 2, 40.f, border / 2));
	shape_summon = new btCylinderShape(btVector3(1.5f / 2, 0.75f, 1.5f / 2));

	Mesh::Point* point = game.aArrow->FindPoint("Empty");
	assert(point && point->IsBox());
	shape_arrow = new btBoxShape(ToVector3(point->size));
}

//=================================================================================================
void Level::Cleanup()
{
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
void Level::Reset()
{
	unit_warp_data.clear();
	to_remove.clear();
}

//=================================================================================================
void Level::ProcessUnitWarps()
{
	Game& game = Game::Get();
	bool warped_to_arena = false;

	for(auto& warp : unit_warp_data)
	{
		if(warp.where == WARP_OUTSIDE)
		{
			if(city_ctx && warp.unit->in_building != -1)
			{
				// exit from building
				InsideBuilding& building = *city_ctx->inside_buildings[warp.unit->in_building];
				RemoveElement(building.units, warp.unit);
				warp.unit->in_building = -1;
				warp.unit->rot = building.outside_rot;
				WarpUnit(*warp.unit, building.outside_spawn);
				local_ctx.units->push_back(warp.unit);
			}
			else
			{
				// unit left location
				if(warp.unit->event_handler)
					warp.unit->event_handler->HandleUnitEvent(UnitEventHandler::LEAVE, warp.unit);
				RemoveUnit(warp.unit);
			}
		}
		else if(warp.where == WARP_ARENA)
		{
			// warp to arena
			InsideBuilding& building = *GetArena();
			RemoveElement(GetContext(*warp.unit).units, warp.unit);
			warp.unit->in_building = building.ctx.building_id;
			Vec3 pos;
			if(!WarpToArea(building.ctx, (warp.unit->in_arena == 0 ? building.arena1 : building.arena2), warp.unit->GetUnitRadius(), pos, 20))
			{
				// failed to exit from arena
				warp.unit->in_building = -1;
				warp.unit->rot = building.outside_rot;
				WarpUnit(*warp.unit, building.outside_spawn);
				local_ctx.units->push_back(warp.unit);
				RemoveElement(game.arena->units, warp.unit);
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
			if(warp.unit->in_building == -1)
				RemoveElement(local_ctx.units, warp.unit);
			else
				RemoveElement(city_ctx->inside_buildings[warp.unit->in_building]->units, warp.unit);
			warp.unit->in_building = warp.where;
			warp.unit->rot = PI;
			WarpUnit(*warp.unit, building.inside_spawn);
			building.units.push_back(warp.unit);
		}

		if(warp.unit == game.pc->unit)
		{
			game.cam.Reset();
			game.pc_data.rot_buf = 0.f;

			if(game.fallback_type == FALLBACK::ARENA)
			{
				game.pc->unit->frozen = FROZEN::ROTATE;
				game.fallback_type = FALLBACK::ARENA2;
			}
			else if(game.fallback_type == FALLBACK::ARENA_EXIT)
			{
				game.pc->unit->frozen = FROZEN::NO;
				game.fallback_type = FALLBACK::NONE;
			}
		}
	}

	unit_warp_data.clear();

	if(warped_to_arena)
	{
		Vec3 pt1(0, 0, 0), pt2(0, 0, 0);
		int count1 = 0, count2 = 0;

		for(vector<Unit*>::iterator it = game.arena->units.begin(), end = game.arena->units.end(); it != end; ++it)
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
			pt1 = ((building.arena1.Midpoint() + building.arena2.Midpoint()) / 2).XZ();
		}
		if(count2 > 0)
			pt2 /= (float)count2;
		else
		{
			InsideBuilding& building = *GetArena();
			pt2 = ((building.arena1.Midpoint() + building.arena2.Midpoint()) / 2).XZ();
		}

		for(vector<Unit*>::iterator it = game.arena->units.begin(), end = game.arena->units.end(); it != end; ++it)
			(*it)->rot = Vec3::LookAtAngle((*it)->pos, (*it)->in_arena == 0 ? pt2 : pt1);
	}
}

//=================================================================================================
void Level::ProcessRemoveUnits(bool leave)
{
	Game& game = Game::Get();
	if(leave)
	{
		for(Unit* unit : to_remove)
		{
			RemoveElement(GetContext(*unit).units, unit);
			delete unit;
		}
	}
	else
	{
		for(Unit* unit : to_remove)
			game.DeleteUnit(unit);
	}
	to_remove.clear();
}

//=================================================================================================
void Level::ApplyContext(ILevel* level, LevelContext& ctx)
{
	assert(level);

	level->ApplyContext(ctx);
	if(ctx.require_tmp_ctx && !ctx.tmp_ctx)
		ctx.SetTmpCtx(tmp_ctx_pool.Get());
}

//=================================================================================================
LevelContext& Level::GetContext(Unit& unit)
{
	if(unit.in_building == -1)
		return local_ctx;
	else
	{
		assert(city_ctx);
		return city_ctx->inside_buildings[unit.in_building]->ctx;
	}
}

//=================================================================================================
LevelContext& Level::GetContext(const Vec3& pos)
{
	if(!city_ctx)
		return local_ctx;
	else
	{
		Int2 offset(int((pos.x - 256.f) / 256.f), int((pos.z - 256.f) / 256.f));
		if(offset.x % 2 == 1)
			++offset.x;
		if(offset.y % 2 == 1)
			++offset.y;
		offset /= 2;
		for(vector<InsideBuilding*>::iterator it = city_ctx->inside_buildings.begin(), end = city_ctx->inside_buildings.end(); it != end; ++it)
		{
			if((*it)->level_shift == offset)
				return (*it)->ctx;
		}
		return local_ctx;
	}
}

//=================================================================================================
LevelContext& Level::GetContextFromInBuilding(int in_building)
{
	if(in_building == -1)
		return local_ctx;
	assert(city_ctx);
	return city_ctx->inside_buildings[in_building]->ctx;
}

//=================================================================================================
Unit* Level::FindUnit(int netid)
{
	if(netid == -1)
		return nullptr;

	for(LevelContext& ctx : ForEachContext())
	{
		for(Unit* unit : *ctx.units)
		{
			if(unit->netid == netid)
				return unit;
		}
	}

	return nullptr;
}

//=================================================================================================
Unit* Level::FindUnit(delegate<bool(Unit*)> pred)
{
	for(LevelContext& ctx : ForEachContext())
	{
		for(Unit* unit : *ctx.units)
		{
			if(pred(unit))
				return unit;
		}
	}

	return nullptr;
}

//=================================================================================================
Usable* Level::FindUsable(int netid)
{
	for(LevelContext& ctx : ForEachContext())
	{
		for(Usable* usable : *ctx.usables)
		{
			if(usable->netid == netid)
				return usable;
		}
	}

	return nullptr;
}

//=================================================================================================
Door* Level::FindDoor(int netid)
{
	for(LevelContext& ctx : ForEachContext())
	{
		if(!ctx.doors)
			continue;
		for(Door* door : *ctx.doors)
		{
			if(door->netid == netid)
				return door;
		}
	}

	return nullptr;
}

//=================================================================================================
Door* Level::FindDoor(LevelContext& ctx, const Int2& pt)
{
	for(vector<Door*>::iterator it = ctx.doors->begin(), end = ctx.doors->end(); it != end; ++it)
	{
		if((*it)->pt == pt)
			return *it;
	}

	return nullptr;
}

//=================================================================================================
Trap* Level::FindTrap(int netid)
{
	if(local_ctx.traps)
	{
		for(vector<Trap*>::iterator it = local_ctx.traps->begin(), end = local_ctx.traps->end(); it != end; ++it)
		{
			if((*it)->netid == netid)
				return *it;
		}
	}

	return nullptr;
}

//=================================================================================================
Chest* Level::FindChest(int netid)
{
	if(local_ctx.chests)
	{
		for(vector<Chest*>::iterator it = local_ctx.chests->begin(), end = local_ctx.chests->end(); it != end; ++it)
		{
			if((*it)->netid == netid)
				return *it;
		}
	}

	return nullptr;
}

//=================================================================================================
Electro* Level::FindElectro(int netid)
{
	for(LevelContext& ctx : ForEachContext())
	{
		for(Electro* electro : *ctx.electros)
		{
			if(electro->netid == netid)
				return electro;
		}
	}

	return nullptr;
}

//=================================================================================================
bool Level::RemoveTrap(int netid)
{
	if(local_ctx.traps)
	{
		for(vector<Trap*>::iterator it = local_ctx.traps->begin(), end = local_ctx.traps->end(); it != end; ++it)
		{
			if((*it)->netid == netid)
			{
				delete *it;
				local_ctx.traps->erase(it);
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
	if(unit->action == A_DESPAWN)
		Game::Get().SpawnUnitEffect(*unit);
	unit->to_remove = true;
	to_remove.push_back(unit);
	if(notify && Net::IsServer())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::REMOVE_UNIT;
		c.id = unit->netid;
	}
}

//=================================================================================================
ObjectEntity Level::SpawnObjectEntity(LevelContext& ctx, BaseObject* base, const Vec3& pos, float rot, float scale, int flags, Vec3* out_point,
	int variant)
{
	if(IS_SET(base->flags, OBJ_TABLE_SPAWNER))
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
		ctx.objects->push_back(o);
		SpawnObjectExtras(ctx, table, pos, rot, o);

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
			ctx.usables->push_back(u);
			u->base = stool;
			u->pos = pos + Vec3(sin(sdir)*slen, 0, cos(sdir)*slen);
			u->rot = sdir;
			u->user = nullptr;
			if(Net::IsOnline())
				u->netid = Usable::netid_counter++;

			SpawnObjectExtras(ctx, stool, u->pos, u->rot, u);
		}

		return o;
	}
	else if(IS_SET(base->flags, OBJ_BUILDING))
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
		ctx.objects->push_back(o);

		ProcessBuildingObjects(ctx, nullptr, nullptr, o->mesh, nullptr, rot, roti, pos, nullptr, nullptr, false, out_point);

		return o;
	}
	else if(IS_SET(base->flags, OBJ_USABLE))
	{
		// usable object
		BaseUsable* base_use = (BaseUsable*)base;

		Usable* u = new Usable;
		u->base = base_use;
		u->pos = pos;
		u->rot = rot;
		u->user = nullptr;

		if(IS_SET(base_use->use_flags, BaseUsable::CONTAINER))
		{
			u->container = new ItemContainer;
			const Item* item = Book::GetRandom();
			if(item)
				u->container->items.push_back({ item, 1, 1 });
		}

		if(variant == -1)
		{
			if(base->variants)
			{
				// extra code for bench
				if(IS_SET(base_use->use_flags, BaseUsable::IS_BENCH))
				{
					switch(location->type)
					{
					case L_CITY:
						variant = 0;
						break;
					case L_DUNGEON:
					case L_CRYPT:
						variant = Rand() % 2;
						break;
					default:
						variant = Rand() % 2 + 2;
						break;
					}
				}
				else
					variant = Random<int>(base->variants->entries.size() - 1);
			}
		}
		u->variant = variant;

		if(Net::IsOnline())
			u->netid = Usable::netid_counter++;
		ctx.usables->push_back(u);

		SpawnObjectExtras(ctx, base, pos, rot, u, scale, flags);

		return u;
	}
	else if(IS_SET(base->flags, OBJ_IS_CHEST))
	{
		// chest
		Chest* chest = new Chest;
		chest->mesh_inst = new MeshInstance(base->mesh);
		chest->rot = rot;
		chest->pos = pos;
		chest->handler = nullptr;
		chest->user = nullptr;
		ctx.chests->push_back(chest);
		if(Net::IsOnline())
			chest->netid = Chest::netid_counter++;

		SpawnObjectExtras(ctx, base, pos, rot, nullptr, scale, flags);

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
		ctx.objects->push_back(o);

		SpawnObjectExtras(ctx, base, pos, rot, o, scale, flags);

		return o;
	}
}

//=================================================================================================
void Level::SpawnObjectExtras(LevelContext& ctx, BaseObject* obj, const Vec3& pos, float rot, void* user_ptr, float scale, int flags)
{
	assert(obj);

	Game& game = Game::Get();

	// ogie� pochodni
	if(!IS_SET(flags, SOE_DONT_SPAWN_PARTICLES))
	{
		if(IS_SET(obj->flags, OBJ_LIGHT))
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
			ctx.pes->push_back(pe);

			pe->tex = tFlare;
			if(IS_SET(obj->flags, OBJ_CAMPFIRE_EFFECT))
				pe->size = 0.7f;
			else
			{
				pe->size = 0.5f;
				if(IS_SET(flags, SOE_MAGIC_LIGHT))
					pe->tex = tFlare2;
			}

			// �wiat�o
			if(!IS_SET(flags, SOE_DONT_CREATE_LIGHT) && ctx.lights)
			{
				Light& s = Add1(ctx.lights);
				s.pos = pe->pos;
				s.range = 5;
				if(IS_SET(flags, SOE_MAGIC_LIGHT))
					s.color = Vec3(0.8f, 0.8f, 1.f);
				else
					s.color = Vec3(1.f, 0.9f, 0.9f);
			}
		}
		else if(IS_SET(obj->flags, OBJ_BLOOD_EFFECT))
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
			pe->tex = game.tKrew[BLOOD_RED];
			pe->size = 0.5f;
			pe->Init();
			ctx.pes->push_back(pe);
		}
		else if(IS_SET(obj->flags, OBJ_WATER_EFFECT))
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
			ctx.pes->push_back(pe);
		}
	}

	// fizyka
	if(obj->shape)
	{
		CollisionObject& c = Add1(ctx.colliders);
		c.ptr = user_ptr;

		int group = CG_OBJECT;
		if(IS_SET(obj->flags, OBJ_PHY_BLOCKS_CAM))
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
			// skalowanie jakim� sposobem przechodzi do btWorldTransform i przy rysowaniu jest z�a skala (dwukrotnie u�yta)
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

		if(IS_SET(obj->flags, OBJ_PHYSICS_PTR))
		{
			assert(user_ptr);
			cobj->setUserPointer(user_ptr);
		}

		if(IS_SET(obj->flags, OBJ_PHY_BLOCKS_CAM))
			c.ptr = CAM_COLLIDER;

		if(IS_SET(obj->flags, OBJ_DOUBLE_PHYSICS))
			SpawnObjectExtras(ctx, obj->next_obj, pos, rot, user_ptr, scale, flags);
		else if(IS_SET(obj->flags, OBJ_MULTI_PHYSICS))
		{
			for(int i = 0;; ++i)
			{
				if(obj->next_obj[i].shape)
					SpawnObjectExtras(ctx, &obj->next_obj[i], pos, rot, user_ptr, scale, flags);
				else
					break;
			}
		}
	}
	else if(IS_SET(obj->flags, OBJ_SCALEABLE))
	{
		CollisionObject& c = Add1(ctx.colliders);
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

	if(IS_SET(obj->flags, OBJ_CAM_COLLIDERS))
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
void Level::ProcessBuildingObjects(LevelContext& ctx, City* city, InsideBuilding* inside, Mesh* mesh, Mesh* inside_mesh, float rot, int roti,
	const Vec3& shift, Building* type, CityBuilding* building, bool recreate, Vec3* out_point)
{
	if(mesh->attach_points.empty())
	{
		building->walk_pt = shift;
		assert(!inside);
		return;
	}

	// https://github.com/Tomash667/carpg/wiki/%5BPL%5D-Buildings-export
	// o_x_[!N!]nazwa_???
	// x (o - obiekt, r - obr�cony obiekt, p - fizyka, s - strefa, c - posta�, m - maska �wiat�a, d - detal wok� obiektu, l - limited rot object)
	// N - wariant (tylko obiekty)
	Game& game = Game::Get();
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
			// obiekt / obr�cony obiekt
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
					if(ctx.type == LevelContext::Outside)
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
					SpawnObjectEntity(ctx, base, pos, r, 1.f, 0, nullptr, variant);
				}
			}
		}
		else if(c == 'p')
		{
			// fizyka
			if(token == "circle" || token == "circlev")
			{
				bool is_wall = (token == "circle");

				CollisionObject& cobj = Add1(ctx.colliders);
				cobj.type = CollisionObject::SPHERE;
				cobj.radius = pt.size.x;
				cobj.pt.x = pos.x;
				cobj.pt.y = pos.z;
				cobj.ptr = (is_wall ? CAM_COLLIDER : nullptr);

				if(ctx.type == LevelContext::Outside)
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

				CollisionObject& cobj = Add1(ctx.colliders);
				cobj.type = CollisionObject::RECTANGLE;
				cobj.pt.x = pos.x;
				cobj.pt.y = pos.z;
				cobj.w = pt.size.x;
				cobj.h = pt.size.z;
				cobj.ptr = (block_camera ? CAM_COLLIDER : nullptr);

				btBoxShape* shape;
				if(token != "squarevp")
				{
					shape = new btBoxShape(btVector3(pt.size.x, 16.f, pt.size.z));
					if(ctx.type == LevelContext::Outside)
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
					if(ctx.type == LevelContext::Outside)
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
				if(ctx.type == LevelContext::Outside)
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
				if(ctx.type == LevelContext::Outside)
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

					inside = new InsideBuilding;
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
					inside->enter_area.v1.x = pos.x - w;
					inside->enter_area.v1.y = pos.z - h;
					inside->enter_area.v2.x = pos.x + w;
					inside->enter_area.v2.y = pos.z + h;
					Vec2 mid = inside->enter_area.Midpoint();
					inside->enter_y = terrain->GetH(mid.x, mid.y) + 0.1f;
					inside->type = type;
					inside->outside_rot = rot;
					inside->top = -1.f;
					inside->xsphere_radius = -1.f;
					ApplyContext(inside, inside->ctx);
					inside->ctx.building_id = (int)city->inside_buildings.size();

					city->inside_buildings.push_back(inside);

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
						inside->ctx.objects->push_back(o);

						ProcessBuildingObjects(inside->ctx, city, inside, inside_mesh, nullptr, 0.f, 0, o->pos, nullptr, nullptr);
					}

					have_exit = true;
				}
				else if(token == "exit")
				{
					assert(inside);

					inside->exit_area.v1.x = pos.x - pt.size.x;
					inside->exit_area.v1.y = pos.z - pt.size.z;
					inside->exit_area.v2.x = pos.x + pt.size.x;
					inside->exit_area.v2.y = pos.z + pt.size.z;

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
					door->pos = pos;
					door->rot = Clip(pt.rot.y + rot);
					door->state = Door::Open;
					door->door2 = (token == "door2");
					door->mesh_inst = new MeshInstance(door->door2 ? game.aDoor2 : game.aDoor);
					door->mesh_inst->groups[0].speed = 2.f;
					door->phy = new btCollisionObject;
					door->phy->setCollisionShape(L.shape_door);
					door->phy->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_DOOR);
					door->locked = LOCK_NONE;
					door->netid = Door::netid_counter++;

					btTransform& tr = door->phy->getWorldTransform();
					Vec3 pos = door->pos;
					pos.y += 1.319f;
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

					ctx.doors->push_back(door);
				}
				else if(token == "arena")
				{
					assert(inside);

					inside->arena1.v1.x = pos.x - pt.size.x;
					inside->arena1.v1.y = pos.z - pt.size.z;
					inside->arena1.v2.x = pos.x + pt.size.x;
					inside->arena1.v2.y = pos.z + pt.size.z;
				}
				else if(token == "arena2")
				{
					assert(inside);

					inside->arena2.v1.x = pos.x - pt.size.x;
					inside->arena2.v1.y = pos.z - pt.size.z;
					inside->arena2.v2.x = pos.x + pt.size.x;
					inside->arena2.v2.y = pos.z + pt.size.z;
				}
				else if(token == "viewer")
				{
					// ten punkt jest u�ywany w SpawnArenaViewers
				}
				else if(token == "point")
				{
					if(building)
					{
						building->walk_pt = pos;
						terrain->SetH(building->walk_pt);
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
					Unit* u = SpawnUnitNearLocation(ctx, pos, *ud, nullptr, -2);
					u->rot = Clip(pt.rot.y + rot);
					u->ai->start_rot = u->rot;
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
			std::random_shuffle(details.begin(), details.end(), MyRand);
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
					if(ctx.type == LevelContext::Outside)
						terrain->SetH(pos);
					SpawnObjectEntity(ctx, obj, pos, Clip(pt.rot.y + rot), 1.f, 0, nullptr, variant);
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

	if(inside)
		inside->ctx.masks = (!inside->masks.empty() ? &inside->masks : nullptr);
}

//=================================================================================================
void Level::RecreateObjects(bool spawn_pes)
{
	for(LevelContext& ctx : ForEachContext())
	{
		int flags = Level::SOE_DONT_CREATE_LIGHT;
		if(!spawn_pes)
			flags |= Level::SOE_DONT_SPAWN_PARTICLES;

		// dotyczy tylko pochodni
		if(ctx.type == LevelContext::Inside)
		{
			InsideLocation* inside = (InsideLocation*)location;
			BaseLocation& base = g_base_locations[inside->target];
			if(IS_SET(base.options, BLO_MAGIC_LIGHT))
				flags |= Level::SOE_MAGIC_LIGHT;
		}

		for(vector<Object*>::iterator it = ctx.objects->begin(), end = ctx.objects->end(); it != end; ++it)
		{
			Object& obj = **it;
			BaseObject* base_obj = obj.base;

			if(!base_obj)
				continue;

			if(IS_SET(base_obj->flags, OBJ_BUILDING))
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

				ProcessBuildingObjects(ctx, nullptr, nullptr, base_obj->mesh, nullptr, rot, roti, obj.pos, nullptr, nullptr, true);
			}
			else
				SpawnObjectExtras(ctx, base_obj, obj.pos, obj.rot.y, &obj, obj.scale, flags);
		}

		if(ctx.chests)
		{
			BaseObject* chest = BaseObject::Get("chest");
			for(vector<Chest*>::iterator it = ctx.chests->begin(), end = ctx.chests->end(); it != end; ++it)
				SpawnObjectExtras(ctx, chest, (*it)->pos, (*it)->rot, nullptr, 1.f, flags);
		}

		for(vector<Usable*>::iterator it = ctx.usables->begin(), end = ctx.usables->end(); it != end; ++it)
			SpawnObjectExtras(ctx, (*it)->base, (*it)->pos, (*it)->rot, *it, 1.f, flags);
	}
}

//=================================================================================================
ObjectEntity Level::SpawnObjectNearLocation(LevelContext& ctx, BaseObject* obj, const Vec2& pos, float rot, float range, float margin, float scale)
{
	bool ok = false;
	if(obj->type == OBJ_CYLINDER)
	{
		global_col.clear();
		Vec3 pt(pos.x, 0, pos.y);
		GatherCollisionObjects(ctx, global_col, pt, obj->r + margin + range);
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

		if(ctx.type == LevelContext::Outside)
			terrain->SetH(pt);

		return SpawnObjectEntity(ctx, obj, pt, rot, scale);
	}
	else
	{
		global_col.clear();
		Vec3 pt(pos.x, 0, pos.y);
		GatherCollisionObjects(ctx, global_col, pt, sqrt(obj->size.x + obj->size.y) + margin + range);
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

		if(ctx.type == LevelContext::Outside)
			terrain->SetH(pt);

		return SpawnObjectEntity(ctx, obj, pt, rot, scale);
	}
}

//=================================================================================================
ObjectEntity Level::SpawnObjectNearLocation(LevelContext& ctx, BaseObject* obj, const Vec2& pos, const Vec2& rot_target, float range, float margin,
	float scale)
{
	if(obj->type == OBJ_CYLINDER)
		return SpawnObjectNearLocation(ctx, obj, pos, Vec2::LookAtAngle(pos, rot_target), range, margin, scale);
	else
	{
		global_col.clear();
		Vec3 pt(pos.x, 0, pos.y);
		GatherCollisionObjects(ctx, global_col, pt, sqrt(obj->size.x + obj->size.y) + margin + range);
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

		if(ctx.type == LevelContext::Outside)
			terrain->SetH(pt);

		return SpawnObjectEntity(ctx, obj, pt, rot, scale);
	}
}

//=================================================================================================
void Level::PickableItemBegin(LevelContext& ctx, Object& o)
{
	assert(o.base);

	pickable_ctx = &ctx;
	pickable_obj = &o;
	pickable_spawns.clear();
	pickable_items.clear();

	for(vector<Mesh::Point>::iterator it = o.base->mesh->attach_points.begin(), end = o.base->mesh->attach_points.end(); it != end; ++it)
	{
		if(strncmp(it->name.c_str(), "spawn_", 6) == 0)
		{
			assert(it->type == Mesh::Point::Box);
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
void Level::AddGroundItem(LevelContext& ctx, GroundItem* item)
{
	assert(item);

	if(ctx.type == LevelContext::Outside)
		terrain->SetH(item->pos);
	ctx.items->push_back(item);

	if(Net::IsOnline())
	{
		item->netid = GroundItem::netid_counter++;
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::SPAWN_ITEM;
		c.item = item;
	}
}

//=================================================================================================
GroundItem* Level::SpawnGroundItemInsideAnyRoom(InsideLocationLevel& lvl, const Item* item)
{
	assert(item);
	while(true)
	{
		int id = Rand() % lvl.rooms.size();
		if(!lvl.rooms[id].IsCorridor())
		{
			GroundItem* item2 = SpawnGroundItemInsideRoom(lvl.rooms[id], item);
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
		GatherCollisionObjects(local_ctx, global_col, pos, 0.25f);
		if(!Collide(global_col, pos, 0.25f))
		{
			GroundItem* gi = new GroundItem;
			gi->count = 1;
			gi->team_count = 1;
			gi->rot = Random(MAX_ANGLE);
			gi->pos = pos;
			gi->item = item;
			gi->netid = GroundItem::netid_counter++;
			local_ctx.items->push_back(gi);
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
		GatherCollisionObjects(local_ctx, global_col, pt, 0.25f);
		if(!Collide(global_col, pt, 0.25f))
		{
			GroundItem* gi = new GroundItem;
			gi->count = 1;
			gi->team_count = 1;
			gi->rot = Random(MAX_ANGLE);
			gi->pos = pt;
			if(local_ctx.type == LevelContext::Outside)
				terrain->SetH(gi->pos);
			gi->item = item;
			gi->netid = GroundItem::netid_counter++;
			local_ctx.items->push_back(gi);
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
		GatherCollisionObjects(local_ctx, global_col, pt, 0.25f);
		if(!Collide(global_col, pt, 0.25f))
		{
			GroundItem* gi = new GroundItem;
			gi->count = 1;
			gi->team_count = 1;
			gi->rot = Random(MAX_ANGLE);
			gi->pos = pt;
			if(local_ctx.type == LevelContext::Outside)
				terrain->SetH(gi->pos);
			gi->item = item;
			gi->netid = GroundItem::netid_counter++;
			local_ctx.items->push_back(gi);
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
			gi->count = 1;
			gi->team_count = 1;
			gi->item = item;
			gi->netid = GroundItem::netid_counter++;
			gi->rot = Random(MAX_ANGLE);
			float rot = pickable_obj->rot.y,
				s = sin(rot),
				c = cos(rot);
			gi->pos = Vec3(pos.x*c + pos.z*s, pos.y, -pos.x*s + pos.z*c) + pickable_obj->pos;
			pickable_ctx->items->push_back(gi);

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

		if(Vec3::Distance(stairs_pos, pt) < 10.f)
			continue;

		Int2 my_pt = Int2(int(pt.x / 2), int(pt.y / 2));
		if(my_pt == stairs_down_pt)
			continue;

		global_col.clear();
		GatherCollisionObjects(local_ctx, global_col, pt, radius, nullptr);

		if(!Collide(global_col, pt, radius))
		{
			float rot = Random(MAX_ANGLE);
			return Game::Get().CreateUnitWithAI(local_ctx, unit, level, nullptr, &pt, &rot);
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

	LocalVector<int> connected(room.connected);
	connected.Shuffle();

	for(vector<int>::iterator it = connected->begin(), end = connected->end(); it != end; ++it)
	{
		u = SpawnUnitInsideRoom(lvl.rooms[*it], ud, level, pt, pt2);
		if(u)
			return u;
	}

	return nullptr;
}

//=================================================================================================
Unit* Level::SpawnUnitNearLocation(LevelContext& ctx, const Vec3 &pos, UnitData &unit, const Vec3* look_at, int level, float extra_radius)
{
	const float radius = unit.GetRadius();

	global_col.clear();
	GatherCollisionObjects(ctx, global_col, pos, extra_radius + radius, nullptr);

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
			return Game::Get().CreateUnitWithAI(ctx, unit, level, nullptr, &tmp_pos, &rot);
		}

		tmp_pos = pos + Vec2::RandomPoissonDiscPoint().XZ() * extra_radius;
	}

	return nullptr;
}

//=================================================================================================
Unit* Level::SpawnUnitInsideArea(LevelContext& ctx, const Box2d& area, UnitData& unit, int level)
{
	Vec3 pos;
	if(!WarpToArea(ctx, area, unit.GetRadius(), pos))
		return nullptr;

	return Game::Get().CreateUnitWithAI(ctx, unit, level, nullptr, &pos);
}

//=================================================================================================
Unit* Level::SpawnUnitInsideInn(UnitData& ud, int level, InsideBuilding* inn, int flags)
{
	Game& game = Game::Get();

	if(!inn)
		inn = city_ctx->FindInn();

	Vec3 pos;
	bool ok = false;
	if(IS_SET(flags, SU_MAIN_ROOM) || Rand() % 5 != 0)
	{
		if(WarpToArea(inn->ctx, inn->arena1, ud.GetRadius(), pos, 20) ||
			WarpToArea(inn->ctx, inn->arena2, ud.GetRadius(), pos, 10))
			ok = true;
	}
	else
	{
		if(WarpToArea(inn->ctx, inn->arena2, ud.GetRadius(), pos, 10) ||
			WarpToArea(inn->ctx, inn->arena1, ud.GetRadius(), pos, 20))
			ok = true;
	}

	if(ok)
	{
		float rot = Random(MAX_ANGLE);
		Unit* u = game.CreateUnitWithAI(inn->ctx, ud, level, nullptr, &pos, &rot);
		if(u && IS_SET(flags, SU_TEMPORARY))
			u->temporary = true;
		return u;
	}
	else
		return nullptr;
}

//=================================================================================================
void Level::SpawnUnitsGroup(LevelContext& ctx, const Vec3& pos, const Vec3* look_at, uint count, UnitGroup* group, int level, delegate<void(Unit*)> callback)
{
	static TmpUnitGroup tgroup;
	tgroup.group = group;
	tgroup.Fill(level);

	for(uint i = 0; i < count; ++i)
	{
		int x = Rand() % tgroup.total,
			y = 0;
		for(auto& entry : tgroup.entries)
		{
			y += entry.count;
			if(x < y)
			{
				Unit* u = SpawnUnitNearLocation(ctx, pos, *entry.ud, look_at, Clamp(entry.ud->level.Random(), level / 2, level), 4.f);
				if(u && callback)
					callback(u);
				break;
			}
		}
	}
}

//=================================================================================================
void Level::GatherCollisionObjects(LevelContext& ctx, vector<CollisionObject>& _objects, const Vec3& _pos, float _radius, const IgnoreObjects* ignore)
{
	assert(_radius > 0.f);

	// tiles
	int minx = max(0, int((_pos.x - _radius) / 2)),
		maxx = int((_pos.x + _radius) / 2),
		minz = max(0, int((_pos.z - _radius) / 2)),
		maxz = int((_pos.z + _radius) / 2);

	if((!ignore || !ignore->ignore_blocks) && ctx.type != LevelContext::Building)
	{
		if(location->outside)
		{
			City* city = (City*)location;
			TerrainTile* tiles = city->tiles;
			maxx = min(maxx, OutsideLocation::size);
			maxz = min(maxz, OutsideLocation::size);

			for(int z = minz; z <= maxz; ++z)
			{
				for(int x = minx; x <= maxx; ++x)
				{
					if(tiles[x + z * OutsideLocation::size].mode >= TM_BUILDING_BLOCK)
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
					POLE co = lvl.map[x + z * lvl.w].type;
					if(czy_blokuje2(co))
					{
						CollisionObject& co = Add1(_objects);
						co.pt = Vec2(2.f*x + 1.f, 2.f*z + 1.f);
						co.w = 1.f;
						co.h = 1.f;
						co.type = CollisionObject::RECTANGLE;
					}
					else if(co == SCHODY_DOL)
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
					else if(co == SCHODY_GORA)
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
	float radius;
	Vec3 pos;
	if(ignore && ignore->ignored_units)
	{
		for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
		{
			if(!*it || !(*it)->IsStanding())
				continue;

			const Unit** u = ignore->ignored_units;
			do
			{
				if(!*u)
					break;
				if(*u == *it)
					goto jest;
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

		jest:
			;
		}
	}
	else
	{
		for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
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

	// obiekty kolizji
	if(ignore && ignore->ignore_objects)
	{
		// ignoruj obiekty
	}
	else if(!ignore || !ignore->ignored_objects)
	{
		for(vector<CollisionObject>::iterator it = ctx.colliders->begin(), end = ctx.colliders->end(); it != end; ++it)
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
		for(vector<CollisionObject>::iterator it = ctx.colliders->begin(), end = ctx.colliders->end(); it != end; ++it)
		{
			if(it->ptr)
			{
				const void** objs = ignore->ignored_objects;
				do
				{
					if(it->ptr == *objs)
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

	// drzwi
	if(ctx.doors && !ctx.doors->empty())
	{
		for(vector<Door*>::iterator it = ctx.doors->begin(), end = ctx.doors->end(); it != end; ++it)
		{
			if((*it)->IsBlocking() && CircleToRotatedRectangle(_pos.x, _pos.z, _radius, (*it)->pos.x, (*it)->pos.z, 0.842f, 0.181f, (*it)->rot))
			{
				CollisionObject& co = Add1(_objects);
				co.pt = Vec2((*it)->pos.x, (*it)->pos.z);
				co.type = CollisionObject::RECTANGLE_ROT;
				co.w = 0.842f;
				co.h = 0.181f;
				co.rot = (*it)->rot;
			}
		}
	}
}

//=================================================================================================
void Level::GatherCollisionObjects(LevelContext& ctx, vector<CollisionObject>& _objects, const Box2d& _box, const IgnoreObjects* ignore)
{
	// tiles
	int minx = max(0, int(_box.v1.x / 2)),
		maxx = int(_box.v2.x / 2),
		minz = max(0, int(_box.v1.y / 2)),
		maxz = int(_box.v2.y / 2);

	if((!ignore || !ignore->ignore_blocks) && ctx.type != LevelContext::Building)
	{
		if(location->outside)
		{
			City* city = (City*)location;
			TerrainTile* tiles = city->tiles;
			maxx = min(maxx, OutsideLocation::size);
			maxz = min(maxz, OutsideLocation::size);

			for(int z = minz; z <= maxz; ++z)
			{
				for(int x = minx; x <= maxx; ++x)
				{
					if(tiles[x + z * OutsideLocation::size].mode >= TM_BUILDING_BLOCK)
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
					POLE co = lvl.map[x + z * lvl.w].type;
					if(czy_blokuje2(co))
					{
						CollisionObject& co = Add1(_objects);
						co.pt = Vec2(2.f*x + 1.f, 2.f*z + 1.f);
						co.w = 1.f;
						co.h = 1.f;
						co.type = CollisionObject::RECTANGLE;
					}
					else if(co == SCHODY_DOL)
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
					else if(co == SCHODY_GORA)
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
	float radius;
	if(ignore && ignore->ignored_units)
	{
		for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
		{
			if(!(*it)->IsStanding())
				continue;

			const Unit** u = ignore->ignored_units;
			do
			{
				if(!*u)
					break;
				if(*u == *it)
					goto jest;
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

		jest:
			;
		}
	}
	else
	{
		for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
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

	// obiekty kolizji
	if(ignore && ignore->ignore_objects)
	{
		// ignoruj obiekty
	}
	else if(!ignore || !ignore->ignored_objects)
	{
		for(vector<CollisionObject>::iterator it = ctx.colliders->begin(), end = ctx.colliders->end(); it != end; ++it)
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
		for(vector<CollisionObject>::iterator it = ctx.colliders->begin(), end = ctx.colliders->end(); it != end; ++it)
		{
			if(it->ptr)
			{
				const void** objs = ignore->ignored_objects;
				do
				{
					if(it->ptr == *objs)
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
void Level::CreateBlood(LevelContext& ctx, const Unit& u, bool fully_created)
{
	Game& game = Game::Get();
	if(!game.tKrewSlad[u.data->blood] || IS_SET(u.data->flags2, F2_BLOODLESS))
		return;

	Blood& b = Add1(ctx.bloods);
	if(u.human_data)
		u.mesh_inst->SetupBones(&u.human_data->mat_scale[0]);
	else
		u.mesh_inst->SetupBones();
	b.pos = u.GetLootCenter();
	b.type = u.data->blood;
	b.rot = Random(MAX_ANGLE);
	b.size = (fully_created ? 1.f : 0.f);

	if(ctx.have_terrain)
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
		CreateBlood(GetContext(*unit), *unit, true);
	blood_to_spawn.clear();
}

//=================================================================================================
void Level::WarpUnit(Unit& unit, const Vec3& pos)
{
	const float unit_radius = unit.GetUnitRadius();

	unit.BreakAction(Unit::BREAK_ACTION_MODE::INSTANT, false, true);

	global_col.clear();
	LevelContext& ctx = GetContext(unit);
	Level::IgnoreObjects ignore = { 0 };
	const Unit* ignore_units[2] = { &unit, nullptr };
	ignore.ignored_units = ignore_units;
	GatherCollisionObjects(ctx, global_col, pos, 2.f + unit_radius, &ignore);

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

	if(ctx.have_terrain && terrain->IsInside(unit.pos))
		terrain->SetH(unit.pos);

	if(unit.cobj)
		unit.UpdatePhysics(unit.pos);

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
bool Level::WarpToArea(LevelContext& ctx, const Box2d& area, float radius, Vec3& pos, int tries)
{
	for(int i = 0; i < tries; ++i)
	{
		pos = area.GetRandomPos3();

		global_col.clear();
		GatherCollisionObjects(ctx, global_col, pos, radius, nullptr);

		if(!Collide(global_col, pos, radius))
			return true;
	}

	return false;
}

//=================================================================================================
// zmienia tylko pozycj� bo ta funkcja jest wywo�ywana przy opuszczaniu miasta
void Level::WarpToInn(Unit& unit)
{
	assert(city_ctx);

	int id;
	InsideBuilding* inn = city_ctx->FindInn(id);

	WarpToArea(inn->ctx, (Rand() % 5 == 0 ? inn->arena2 : inn->arena1), unit.GetUnitRadius(), unit.pos, 20);
	unit.visual_pos = unit.pos;
	unit.in_building = id;
}

//=================================================================================================
void Level::WarpNearLocation(LevelContext& ctx, Unit& unit, const Vec3& pos, float extra_radius, bool allow_exact, int tries)
{
	const float radius = unit.GetUnitRadius();

	global_col.clear();
	IgnoreObjects ignore = { 0 };
	const Unit* ignore_units[2] = { &unit, nullptr };
	ignore.ignored_units = ignore_units;
	GatherCollisionObjects(ctx, global_col, pos, extra_radius + radius, &ignore);

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
	Game::Get().MoveUnit(unit, true);
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
		unit.UpdatePhysics(unit.pos);
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
	local_ctx.traps->push_back(t);

	auto& base = BaseTrap::traps[type];
	trap.base = &base;
	trap.hitted = nullptr;
	trap.state = 0;
	trap.pos = Vec3(2.f*pt.x + Random(trap.base->rw, 2.f - trap.base->rw), 0.f, 2.f*pt.y + Random(trap.base->h, 2.f - trap.base->h));
	trap.obj.base = nullptr;
	trap.obj.mesh = trap.base->mesh;
	trap.obj.pos = trap.pos;
	trap.obj.scale = 1.f;
	trap.netid = Trap::netid_counter++;

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
				if(czy_blokuje2(lvl.map[pt.x + DirToPos(dir).x*j + (pt.y + DirToPos(dir).y*j)*lvl.w]))
				{
					if(j != 1)
						ok = true;
					break;
				}
			}

			if(ok)
			{
				trap.tile = pt + DirToPos(dir) * j;

				if(Game::Get().CanShootAtLocation(Vec3(trap.pos.x + (2.f*j - 1.2f)*DirToPos(dir).x, 1.f, trap.pos.z + (2.f*j - 1.2f)*DirToPos(dir).y),
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
			local_ctx.traps->pop_back();
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
		trap.obj.base = &Game::Get().obj_alpha;
	}

	if(timed)
	{
		trap.state = -1;
		trap.time = 2.f;
	}

	return &trap;
}

//=================================================================================================
void Level::UpdateLocation(int days, int open_chance, bool reset)
{
	// up�yw czasu
	// 1-10 dni (usu� zw�oki)
	// 5-30 dni (usu� krew)
	// 1+ zamknij/otw�rz drzwi
	for(LevelContext& ctx : ForEachContext())
	{
		if(days <= 10)
		{
			// usu� niekt�re zw�oki i przedmioty
			for(Unit*& u : *ctx.units)
			{
				if(!u->IsAlive() && Random(4, 10) < days)
				{
					delete u;
					u = nullptr;
				}
			}
			RemoveNullElements(ctx.units);
			auto from = std::remove_if(ctx.items->begin(), ctx.items->end(), RemoveRandomPred<GroundItem*>(days, 0, 10));
			auto end = ctx.items->end();
			if(from != end)
			{
				for(vector<GroundItem*>::iterator it = from; it != end; ++it)
					delete *it;
				ctx.items->erase(from, end);
			}
		}
		else
		{
			// usu� wszystkie zw�oki i przedmioty
			if(reset)
			{
				for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
					delete *it;
				ctx.units->clear();
			}
			else
			{
				for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
				{
					if(!(*it)->IsAlive())
					{
						delete *it;
						*it = nullptr;
					}
				}
				RemoveNullElements(ctx.units);
			}
			DeleteElements(ctx.items);
		}

		// wylecz jednostki
		if(!reset)
		{
			for(vector<Unit*>::iterator it = ctx.units->begin(), end = ctx.units->end(); it != end; ++it)
			{
				if((*it)->IsAlive())
					(*it)->NaturalHealing(days);
			}
		}

		if(days > 30)
		{
			// usu� krew
			ctx.bloods->clear();
		}
		else if(days >= 5)
		{
			// usu� cz�ciowo krew
			RemoveElements(ctx.bloods, RemoveRandomPred<Blood>(days, 4, 30));
		}

		if(ctx.traps)
		{
			if(days > 30)
			{
				// usu� wszystkie jednorazowe pu�apki
				for(vector<Trap*>::iterator it = ctx.traps->begin(), end = ctx.traps->end(); it != end;)
				{
					if((*it)->base->type == TRAP_FIREBALL)
					{
						delete *it;
						if(it + 1 == end)
						{
							ctx.traps->pop_back();
							break;
						}
						else
						{
							std::iter_swap(it, end - 1);
							ctx.traps->pop_back();
							end = ctx.traps->end();
						}
					}
					else
						++it;
				}
			}
			else if(days >= 5)
			{
				// usu� cz�� 1razowych pu�apek
				for(vector<Trap*>::iterator it = ctx.traps->begin(), end = ctx.traps->end(); it != end;)
				{
					if((*it)->base->type == TRAP_FIREBALL && Rand() % 30 < days)
					{
						delete *it;
						if(it + 1 == end)
						{
							ctx.traps->pop_back();
							break;
						}
						else
						{
							std::iter_swap(it, end - 1);
							ctx.traps->pop_back();
							end = ctx.traps->end();
						}
					}
					else
						++it;
				}
			}
		}

		// losowo otw�rz/zamknij drzwi
		if(ctx.doors)
		{
			for(vector<Door*>::iterator it = ctx.doors->begin(), end = ctx.doors->end(); it != end; ++it)
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
}

//=================================================================================================
int Level::GetDifficultyLevel() const
{
	if(location->outside)
		return location->st;
	else
	{
		InsideLocation* inside = (InsideLocation*)location;
		if(inside->IsMultilevel())
			return (int)Lerp(max(3.f, float(inside->st) / 2), float(inside->st), float(dungeon_level) / (((MultiInsideLocation*)inside)->levels.size() - 1));
		else
			return inside->st;
	}
}

//=================================================================================================
int Level::GetChestDifficultyLevel() const
{
	int st = GetDifficultyLevel();
	if(location->spawn == SG_NONE)
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
	Game& game = Game::Get();

	for(LevelContext& ctx : ForEachContext())
	{
		// odtw�rz skrzynie
		if(ctx.chests)
		{
			for(vector<Chest*>::iterator it = ctx.chests->begin(), end = ctx.chests->end(); it != end; ++it)
			{
				Chest& chest = **it;

				chest.mesh_inst = new MeshInstance(game.aChest);
			}
		}

		// odtw�rz drzwi
		if(ctx.doors)
		{
			for(vector<Door*>::iterator it = ctx.doors->begin(), end = ctx.doors->end(); it != end; ++it)
			{
				Door& door = **it;

				// animowany model
				door.mesh_inst = new MeshInstance(door.door2 ? game.aDoor2 : game.aDoor);
				door.mesh_inst->groups[0].speed = 2.f;

				// fizyka
				door.phy = new btCollisionObject;
				door.phy->setCollisionShape(L.shape_door);
				door.phy->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_DOOR);
				btTransform& tr = door.phy->getWorldTransform();
				Vec3 pos = door.pos;
				pos.y += 1.319f;
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
}

//=================================================================================================
InsideBuilding* Level::GetArena()
{
	assert(city_ctx);
	for(InsideBuilding* b : city_ctx->inside_buildings)
	{
		if(b->type->group == BuildingGroup::BG_ARENA)
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
	else
		return Format(txLocationTextMap, location->name.c_str());
}

//=================================================================================================
void Level::CheckIfLocationCleared()
{
	if(city_ctx)
		return;

	Game& game = Game::Get();
	bool is_clear = true;
	for(vector<Unit*>::iterator it = local_ctx.units->begin(), end = local_ctx.units->end(); it != end; ++it)
	{
		if((*it)->IsAlive() && game.pc->unit->IsEnemy(**it, true))
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
			InsideLocation* inside = (InsideLocation*)location;
			if(inside->IsMultilevel())
			{
				if(((MultiInsideLocation*)inside)->LevelCleared())
					cleared = true;
			}
			else
				cleared = true;
		}
		else
			cleared = true;

		if(cleared)
			location->state = LS_CLEARED;

		bool prevent = false;
		if(event_handler)
			prevent = event_handler->HandleLocationEvent(LocationEventHandler::CLEARED);

		if(cleared && prevent && location->spawn != SG_NONE)
		{
			if(location->type == L_CAMP)
				W.AddNews(Format(txNewsCampCleared, W.GetLocation(W.GetNearestSettlement(location->pos))->name.c_str()));
			else
				W.AddNews(Format(txNewsLocCleared, location->name.c_str()));
		}
	}
}

//=================================================================================================
// usuwa podany przedmiot ze �wiata
// u�ywane w que�cie z kamieniem szale�c�w
bool Level::RemoveItemFromWorld(const Item* item)
{
	assert(item);
	for(LevelContext& ctx : ForEachContext())
	{
		if(ctx.RemoveItemFromWorld(item))
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
	Pole* m = lvl.map;
	int w = lvl.w,
		h = lvl.h;

	for(int y = 1; y < h - 1; ++y)
	{
		for(int x = 1; x < w - 1; ++x)
		{
			if(czy_blokuje2(m[x + y * w]) && (!czy_blokuje2(m[x - 1 + (y - 1)*w]) || !czy_blokuje2(m[x + (y - 1)*w]) || !czy_blokuje2(m[x + 1 + (y - 1)*w]) ||
				!czy_blokuje2(m[x - 1 + y * w]) || !czy_blokuje2(m[x + 1 + y * w]) ||
				!czy_blokuje2(m[x - 1 + (y + 1)*w]) || !czy_blokuje2(m[x + (y + 1)*w]) || !czy_blokuje2(m[x + 1 + (y + 1)*w])))
			{
				SpawnDungeonCollider(Vec3(2.f*x + 1.f, 2.f, 2.f*y + 1.f));
			}
		}
	}

	// left/right wall
	for(int i = 1; i < h - 1; ++i)
	{
		// left
		if(czy_blokuje2(m[i*w]) && !czy_blokuje2(m[1 + i * w]))
			SpawnDungeonCollider(Vec3(1.f, 2.f, 2.f*i + 1.f));

		// right
		if(czy_blokuje2(m[i*w + w - 1]) && !czy_blokuje2(m[w - 2 + i * w]))
			SpawnDungeonCollider(Vec3(2.f*(w - 1) + 1.f, 2.f, 2.f*i + 1.f));
	}

	// front/back wall
	for(int i = 1; i < lvl.w - 1; ++i)
	{
		// front
		if(czy_blokuje2(m[i + (h - 1)*w]) && !czy_blokuje2(m[i + (h - 2)*w]))
			SpawnDungeonCollider(Vec3(2.f*i + 1.f, 2.f, 2.f*(h - 1) + 1.f));

		// back
		if(czy_blokuje2(m[i]) && !czy_blokuje2(m[i + w]))
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

	if((inside->type == L_DUNGEON && inside->target == LABIRYNTH) || inside->type == L_CAVE)
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

	for(Room& room : lvl.rooms)
	{
		// floor
		dungeon_shape_pos.push_back(Vec3(2.f * room.pos.x, 0, 2.f * room.pos.y));
		dungeon_shape_pos.push_back(Vec3(2.f * (room.pos.x + room.size.x), 0, 2.f * room.pos.y));
		dungeon_shape_pos.push_back(Vec3(2.f * room.pos.x, 0, 2.f * (room.pos.y + room.size.y)));
		dungeon_shape_pos.push_back(Vec3(2.f * (room.pos.x + room.size.x), 0, 2.f * (room.pos.y + room.size.y)));
		dungeon_shape_index.push_back(index);
		dungeon_shape_index.push_back(index + 1);
		dungeon_shape_index.push_back(index + 2);
		dungeon_shape_index.push_back(index + 2);
		dungeon_shape_index.push_back(index + 1);
		dungeon_shape_index.push_back(index + 3);
		index += 4;

		// ceil
		const float h = (room.IsCorridor() ? Room::HEIGHT_LOW : Room::HEIGHT);
		dungeon_shape_pos.push_back(Vec3(2.f * room.pos.x, h, 2.f * room.pos.y));
		dungeon_shape_pos.push_back(Vec3(2.f * (room.pos.x + room.size.x), h, 2.f * room.pos.y));
		dungeon_shape_pos.push_back(Vec3(2.f * room.pos.x, h, 2.f * (room.pos.y + room.size.y)));
		dungeon_shape_pos.push_back(Vec3(2.f * (room.pos.x + room.size.x), h, 2.f * (room.pos.y + room.size.y)));
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
