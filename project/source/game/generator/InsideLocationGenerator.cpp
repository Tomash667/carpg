#include "Pch.h"
#include "GameCore.h"
#include "InsideLocationGenerator.h"
#include "MultiInsideLocation.h"
#include "OutsideLocation.h"
#include "World.h"
#include "Level.h"
#include "QuestManager.h"
#include "Quest_Orcs.h"
#include "Quest_Secret.h"
#include "Stock.h"
#include "Debug.h"
#include "Portal.h"
#include "Texture.h"
#include "GameStats.h"
#include "RoomType.h"
#include "ItemHelper.h"
#include "Game.h"

//=================================================================================================
void InsideLocationGenerator::Init()
{
	inside = (InsideLocation*)loc;
}

//=================================================================================================
int InsideLocationGenerator::GetNumberOfSteps()
{
	int steps = LocationGenerator::GetNumberOfSteps();
	if(first)
		steps += 2; // txGeneratingObjects, txGeneratingUnits
	else if(!reenter)
		++steps; // txRegeneratingLevel
	return steps;
}

//=================================================================================================
void InsideLocationGenerator::OnEnter()
{
	Game& game = Game::Get();
	inside->SetActiveLevel(dungeon_level);
	int days;
	bool need_reset = inside->CheckUpdate(days, W.GetWorldtime());
	InsideLocationLevel& lvl = inside->GetLevelData();
	BaseLocation& base = g_base_locations[inside->target];

	if(!reenter)
		L.ApplyContext(inside, L.local_ctx);

	game.SetDungeonParamsAndTextures(base);

	if(first)
	{
		game.LoadingStep(game.txGeneratingObjects);
		GenerateObjects();

		game.LoadingStep(game.txGeneratingUnits);
		GenerateUnits();

		GenerateItems();
	}
	else if(!reenter)
	{
		game.LoadingStep(game.txRegeneratingLevel);

		if(days > 0)
			L.UpdateLocation(days, base.door_open, need_reset);

		bool respawn_units = HandleUpdate(days);

		if(need_reset)
		{
			// usu� �ywe jednostki
			if(days != 0)
			{
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

			// usu� zawarto�� skrzyni
			for(vector<Chest*>::iterator it = L.local_ctx.chests->begin(), end = L.local_ctx.chests->end(); it != end; ++it)
				(*it)->items.clear();

			// nowa zawarto�� skrzyni
			int dlevel = L.GetDifficultyLevel();
			for(vector<Chest*>::iterator it = L.local_ctx.chests->begin(), end = L.local_ctx.chests->end(); it != end; ++it)
			{
				Chest& chest = **it;
				if(!chest.items.empty())
					continue;
				Room* r = lvl.GetNearestRoom(chest.pos);
				static vector<Chest*> room_chests;
				room_chests.push_back(&chest);
				for(vector<Chest*>::iterator it2 = it + 1; it2 != end; ++it2)
				{
					if(r->IsInside((*it2)->pos))
						room_chests.push_back(*it2);
				}
				GenerateDungeonTreasure(room_chests, dlevel);
				room_chests.clear();
			}

			// nowe jednorazowe pu�apki
			RegenerateTraps();
		}

		L.OnReenterLevel();

		// odtw�rz jednostki
		if(respawn_units)
			RespawnUnits();
		RespawnTraps();

		// odtw�rz fizyk�
		L.RecreateObjects();

		if(need_reset)
			GenerateUnits();
	}

	// questowe rzeczy
	if(inside->active_quest && inside->active_quest != (Quest_Dungeon*)ACTIVE_QUEST_HOLDER)
	{
		Quest_Event* event = inside->active_quest->GetEvent(L.location_index);
		if(event)
		{
			if(event->at_level == dungeon_level)
			{
				if(!event->done)
				{
					game.HandleQuestEvent(event);

					// generowanie ork�w
					if(L.location_index == QM.quest_orcs2->target_loc && QM.quest_orcs2->orcs_state == Quest_Orcs2::State::GenerateOrcs)
					{
						QM.quest_orcs2->orcs_state = Quest_Orcs2::State::GeneratedOrcs;
						UnitData* ud = UnitData::Get("q_orkowie_slaby");
						for(vector<Room>::iterator it = lvl.rooms.begin(), end = lvl.rooms.end(); it != end; ++it)
						{
							if(!it->IsCorridor() && Rand() % 2 == 0)
							{
								Unit* u = L.SpawnUnitInsideRoom(*it, *ud, -2, Int2(-999, -999), Int2(-999, -999));
								if(u)
									u->dont_attack = true;
							}
						}

						Unit* u = L.SpawnUnitInsideRoom(lvl.GetFarRoom(false), *UnitData::Get("q_orkowie_kowal"), -2, Int2(-999, -999), Int2(-999, -999));
						if(u)
							u->dont_attack = true;

						vector<ItemSlot>& items = QM.quest_orcs2->wares;
						Stock::Get("orc_blacksmith")->Parse(0, false, items);
						SortItems(items);
					}
				}

				L.event_handler = event->location_event_handler;
			}
			else if(inside->active_quest->whole_location_event_handler)
				L.event_handler = event->location_event_handler;
		}
	}

	if((first || need_reset) && (Rand() % 50 == 0 || DebugKey('C')) && L.location->type != L_CAVE && inside->target != LABIRYNTH
		&& !L.location->active_quest && dungeon_level == 0)
		SpawnHeroesInsideDungeon();

	// stw�rz obiekty kolizji
	if(!reenter)
		L.SpawnDungeonColliders();

	// generuj minimap�
	game.LoadingStep(game.txGeneratingMinimap);
	CreateMinimap();

	// sekret
	Quest_Secret* secret = QM.quest_secret;
	if(L.location_index == secret->where && !inside->HaveDownStairs() && secret->state == Quest_Secret::SECRET_DROPPED_STONE)
	{
		secret->state = Quest_Secret::SECRET_GENERATED;
		if(game.devmode)
			Info("Generated secret room.");

		Room& r = GetLevelData().rooms[0];

		if(game.hardcore_mode)
		{
			Object* o = L.local_ctx.FindObject(BaseObject::Get("portal"));

			OutsideLocation* loc = new OutsideLocation;
			loc->active_quest = (Quest_Dungeon*)ACTIVE_QUEST_HOLDER;
			loc->pos = Vec2(-999, -999);
			loc->st = 20;
			loc->name = game.txHiddenPlace;
			loc->type = L_FOREST;
			loc->image = LI_FOREST;
			int loc_id = W.AddLocation(loc);

			Portal* portal = new Portal;
			portal->at_level = 2;
			portal->next_portal = nullptr;
			portal->pos = o->pos;
			portal->rot = o->rot.y;
			portal->target = 0;
			portal->target_loc = loc_id;

			inside->portal = portal;
			secret->where2 = loc_id;
		}
		else
		{
			// dodaj kartk� (overkill sprawdzania!)
			const Item* kartka = Item::Get("sekret_kartka2");
			assert(kartka);
			Chest* c = L.local_ctx.FindChestInRoom(r);
			assert(c);
			if(c)
				c->AddItem(kartka, 1, 1);
			else
			{
				Object* o = L.local_ctx.FindObject(BaseObject::Get("portal"));
				assert(0);
				if(o)
				{
					GroundItem* item = new GroundItem;
					item->count = 1;
					item->team_count = 1;
					item->item = kartka;
					item->netid = GroundItem::netid_counter++;
					item->pos = o->pos;
					item->rot = Random(MAX_ANGLE);
					L.local_ctx.items->push_back(item);
				}
				else
					L.SpawnGroundItemInsideRoom(r, kartka);
			}

			secret->state = Quest_Secret::SECRET_CLOSED;
		}
	}

	// dodaj gracza i jego dru�yn�
	Int2 spawn_pt;
	Vec3 spawn_pos;
	float spawn_rot;

	if(L.enter_from < ENTER_FROM_PORTAL)
	{
		if(L.enter_from == ENTER_FROM_DOWN_LEVEL)
		{
			spawn_pt = lvl.GetDownStairsFrontTile();
			spawn_rot = DirToRot(lvl.staircase_down_dir);
		}
		else
		{
			spawn_pt = lvl.GetUpStairsFrontTile();
			spawn_rot = DirToRot(lvl.staircase_up_dir);
		}
		spawn_pos = PtToPos(spawn_pt);
	}
	else
	{
		Portal* portal = inside->GetPortal(L.enter_from);
		spawn_pos = portal->GetSpawnPos();
		spawn_rot = Clip(portal->rot + PI);
		spawn_pt = PosToPt(spawn_pos);
	}

	L.AddPlayerTeam(spawn_pos, spawn_rot, reenter, L.enter_from == ENTER_FROM_OUTSIDE);
	game.OpenDoorsByTeam(spawn_pt);
}

//=================================================================================================
InsideLocationLevel& InsideLocationGenerator::GetLevelData()
{
	return inside->GetLevelData();
}

//=================================================================================================
void InsideLocationGenerator::AddRoomColliders(InsideLocationLevel& lvl, Room& room, vector<Int2>& blocks)
{
	// add colliding blocks
	for(int x = 0; x < room.size.x; ++x)
	{
		// top
		TILE_TYPE co = lvl.map[room.pos.x + x + (room.pos.y + room.size.y - 1)*lvl.w].type;
		if(co == EMPTY || co == BARS || co == BARS_FLOOR || co == BARS_CEILING || co == DOORS || co == HOLE_FOR_DOORS)
		{
			blocks.push_back(Int2(room.pos.x + x, room.pos.y + room.size.y - 1));
			blocks.push_back(Int2(room.pos.x + x, room.pos.y + room.size.y - 2));
		}
		else if(co == WALL || co == BLOCKADE_WALL)
			blocks.push_back(Int2(room.pos.x + x, room.pos.y + room.size.y - 1));

		// bottom
		co = lvl.map[room.pos.x + x + room.pos.y*lvl.w].type;
		if(co == EMPTY || co == BARS || co == BARS_FLOOR || co == BARS_CEILING || co == DOORS || co == HOLE_FOR_DOORS)
		{
			blocks.push_back(Int2(room.pos.x + x, room.pos.y));
			blocks.push_back(Int2(room.pos.x + x, room.pos.y + 1));
		}
		else if(co == WALL || co == BLOCKADE_WALL)
			blocks.push_back(Int2(room.pos.x + x, room.pos.y));
	}
	for(int y = 0; y < room.size.y; ++y)
	{
		// left
		TILE_TYPE co = lvl.map[room.pos.x + (room.pos.y + y)*lvl.w].type;
		if(co == EMPTY || co == BARS || co == BARS_FLOOR || co == BARS_CEILING || co == DOORS || co == HOLE_FOR_DOORS)
		{
			blocks.push_back(Int2(room.pos.x, room.pos.y + y));
			blocks.push_back(Int2(room.pos.x + 1, room.pos.y + y));
		}
		else if(co == WALL || co == BLOCKADE_WALL)
			blocks.push_back(Int2(room.pos.x, room.pos.y + y));

		// right
		co = lvl.map[room.pos.x + room.size.x - 1 + (room.pos.y + y)*lvl.w].type;
		if(co == EMPTY || co == BARS || co == BARS_FLOOR || co == BARS_CEILING || co == DOORS || co == HOLE_FOR_DOORS)
		{
			blocks.push_back(Int2(room.pos.x + room.size.x - 1, room.pos.y + y));
			blocks.push_back(Int2(room.pos.x + room.size.x - 2, room.pos.y + y));
		}
		else if(co == WALL || co == BLOCKADE_WALL)
			blocks.push_back(Int2(room.pos.x + room.size.x - 1, room.pos.y + y));
	}
}

//=================================================================================================
void InsideLocationGenerator::GenerateDungeonObjects()
{
	Game& game = Game::Get();
	InsideLocationLevel& lvl = GetLevelData();
	BaseLocation& base = g_base_locations[inside->target];
	static vector<Chest*> room_chests;
	static vector<Vec3> on_wall;
	static vector<Int2> blocks;
	int chest_lvl = L.GetChestDifficultyLevel();

	// schody w g�r�
	if(inside->HaveUpStairs())
	{
		Object* o = new Object;
		o->mesh = game.aStairsUp;
		o->pos = PtToPos(lvl.staircase_up);
		o->rot = Vec3(0, DirToRot(lvl.staircase_up_dir), 0);
		o->scale = 1;
		o->base = nullptr;
		L.local_ctx.objects->push_back(o);
	}
	else
		L.SpawnObjectEntity(L.local_ctx, BaseObject::Get("portal"), inside->portal->pos, inside->portal->rot);

	// schody w d�
	if(inside->HaveDownStairs())
	{
		Object* o = new Object;
		o->mesh = (lvl.staircase_down_in_wall ? game.aStairsDown2 : game.aStairsDown);
		o->pos = PtToPos(lvl.staircase_down);
		o->rot = Vec3(0, DirToRot(lvl.staircase_down_dir), 0);
		o->scale = 1;
		o->base = nullptr;
		L.local_ctx.objects->push_back(o);
	}

	// kratki, drzwi
	for(int y = 0; y < lvl.h; ++y)
	{
		for(int x = 0; x < lvl.w; ++x)
		{
			TILE_TYPE p = lvl.map[x + y * lvl.w].type;
			if(p == BARS || p == BARS_FLOOR)
			{
				Object* o = new Object;
				o->mesh = game.aGrating;
				o->rot = Vec3(0, 0, 0);
				o->pos = Vec3(float(x * 2), 0, float(y * 2));
				o->scale = 1;
				o->base = nullptr;
				L.local_ctx.objects->push_back(o);
			}
			if(p == BARS || p == BARS_CEILING)
			{
				Object* o = new Object;
				o->mesh = game.aGrating;
				o->rot = Vec3(0, 0, 0);
				o->pos = Vec3(float(x * 2), 4, float(y * 2));
				o->scale = 1;
				o->base = nullptr;
				L.local_ctx.objects->push_back(o);
			}
			if(p == DOORS)
			{
				Object* o = new Object;
				o->mesh = game.aDoorWall;
				if(IS_SET(lvl.map[x + y * lvl.w].flags, Tile::F_SECOND_TEXTURE))
					o->mesh = game.aDoorWall2;
				o->pos = Vec3(float(x * 2) + 1, 0, float(y * 2) + 1);
				o->scale = 1;
				o->base = nullptr;
				L.local_ctx.objects->push_back(o);

				if(IsBlocking(lvl.map[x - 1 + y * lvl.w].type))
				{
					o->rot = Vec3(0, 0, 0);
					int mov = 0;
					if(lvl.rooms[lvl.map[x + (y - 1)*lvl.w].room].IsCorridor())
						++mov;
					if(lvl.rooms[lvl.map[x + (y + 1)*lvl.w].room].IsCorridor())
						--mov;
					if(mov == 1)
						o->pos.z += 0.8229f;
					else if(mov == -1)
						o->pos.z -= 0.8229f;
				}
				else
				{
					o->rot = Vec3(0, PI / 2, 0);
					int mov = 0;
					if(lvl.rooms[lvl.map[x - 1 + y * lvl.w].room].IsCorridor())
						++mov;
					if(lvl.rooms[lvl.map[x + 1 + y * lvl.w].room].IsCorridor())
						--mov;
					if(mov == 1)
						o->pos.x += 0.8229f;
					else if(mov == -1)
						o->pos.x -= 0.8229f;
				}

				if(Rand() % 100 < base.door_chance || IS_SET(lvl.map[x + y * lvl.w].flags, Tile::F_SPECIAL))
				{
					Door* door = new Door;
					L.local_ctx.doors->push_back(door);
					door->pt = Int2(x, y);
					door->pos = o->pos;
					door->rot = o->rot.y;
					door->state = Door::Closed;
					door->mesh_inst = new MeshInstance(game.aDoor);
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
					game.phy_world->addCollisionObject(door->phy, CG_DOOR);

					if(IS_SET(lvl.map[x + y * lvl.w].flags, Tile::F_SPECIAL))
						door->locked = LOCK_ORCS;
					else if(Rand() % 100 < base.door_open)
					{
						door->state = Door::Open;
						btVector3& pos = door->phy->getWorldTransform().getOrigin();
						pos.setY(pos.y() - 100.f);
						door->mesh_inst->SetToEnd(door->mesh_inst->mesh->anims[0].name.c_str());
					}
				}
				else
					lvl.map[x + y * lvl.w].type = HOLE_FOR_DOORS;
			}
		}
	}

	// dotyczy tylko pochodni
	int flags = 0;
	if(IS_SET(base.options, BLO_MAGIC_LIGHT))
		flags = Level::SOE_MAGIC_LIGHT;

	bool wymagany = false;
	if(base.wymagany.room)
		wymagany = true;

	for(vector<Room>::iterator it = lvl.rooms.begin(), end = lvl.rooms.end(); it != end; ++it)
	{
		if(it->IsCorridor())
			continue;

		AddRoomColliders(lvl, *it, blocks);

		// choose room type
		RoomType* rt;
		if(it->target != RoomTarget::None)
		{
			if(it->target == RoomTarget::Treasury)
				rt = RoomType::Find("krypta_skarb");
			else if(it->target == RoomTarget::Throne)
				rt = RoomType::Find("tron");
			else if(it->target == RoomTarget::PortalCreate)
				rt = RoomType::Find("portal");
			else
			{
				Int2 pt;
				if(it->target == RoomTarget::StairsDown)
					pt = lvl.staircase_down;
				else if(it->target == RoomTarget::StairsUp)
					pt = lvl.staircase_up;
				else if(it->target == RoomTarget::Portal)
				{
					if(inside->portal)
						pt = PosToPt(inside->portal->pos);
					else
					{
						Object* o = L.local_ctx.FindObject(BaseObject::Get("portal"));
						if(o)
							pt = PosToPt(o->pos);
						else
							pt = it->CenterTile();
					}
				}

				for(int y = -1; y <= 1; ++y)
				{
					for(int x = -1; x <= 1; ++x)
						blocks.push_back(Int2(pt.x + x, pt.y + y));
				}

				if(base.schody.room)
					rt = base.schody.room;
				else
					rt = base.GetRandomRoomType();
			}
		}
		else
		{
			if(wymagany)
				rt = base.wymagany.room;
			else
				rt = base.GetRandomRoomType();
		}

		int fail = 10;
		bool wymagany_obiekt = false;

		// try to spawn all objects
		for(uint i = 0; i < rt->count && fail > 0; ++i)
		{
			bool is_group = false;
			BaseObject* base = BaseObject::Get(rt->objs[i].id, &is_group);
			int count = rt->objs[i].count.Random();

			for(int j = 0; j < count && fail > 0; ++j)
			{
				auto e = GenerateDungeonObject(lvl, *it, base, on_wall, blocks, flags);
				if(!e)
				{
					if(IS_SET(base->flags, OBJ_IMPORTANT))
						--j;
					--fail;
					continue;
				}

				if(e.type == ObjectEntity::CHEST)
					room_chests.push_back(e);

				if(IS_SET(base->flags, OBJ_REQUIRED))
					wymagany_obiekt = true;

				if(is_group)
					base = BaseObject::Get(rt->objs[i].id);
			}
		}

		if(wymagany && wymagany_obiekt && it->target == RoomTarget::None)
			wymagany = false;

		if(!room_chests.empty())
		{
			bool extra = IS_SET(rt->flags, RT_TREASURE);
			GenerateDungeonTreasure(room_chests, chest_lvl, extra);
			room_chests.clear();
		}

		on_wall.clear();
		blocks.clear();
	}

	if(wymagany)
		throw "Failed to generate required object!";

	if(L.local_ctx.chests->empty())
	{
		// niech w podziemiach b�dzie minimum 1 skrzynia
		BaseObject* base = BaseObject::Get("chest");
		for(int i = 0; i < 100; ++i)
		{
			on_wall.clear();
			blocks.clear();
			Room& r = lvl.rooms[Rand() % lvl.rooms.size()];
			if(r.target == RoomTarget::None)
			{
				AddRoomColliders(lvl, r, blocks);

				auto e = GenerateDungeonObject(lvl, r, base, on_wall, blocks, flags);
				if(e)
				{
					GenerateDungeonTreasure(*L.local_ctx.chests, chest_lvl);
					break;
				}
			}
		}
	}
}

//=================================================================================================
ObjectEntity InsideLocationGenerator::GenerateDungeonObject(InsideLocationLevel& lvl, Room& room, BaseObject* base, vector<Vec3>& on_wall,
	vector<Int2>& blocks, int flags)
{
	Vec3 pos;
	float rot;
	Vec2 shift;

	if(base->type == OBJ_CYLINDER)
	{
		shift.x = base->r + base->extra_dist;
		shift.y = base->r + base->extra_dist;
	}
	else
		shift = base->size + Vec2(base->extra_dist, base->extra_dist);

	if(IS_SET(base->flags, OBJ_NEAR_WALL))
	{
		Int2 tile;
		GameDirection dir;
		if(!lvl.GetRandomNearWallTile(room, tile, dir, IS_SET(base->flags, OBJ_ON_WALL)))
			return nullptr;

		rot = DirToRot(dir);
		if(dir == 2 || dir == 3)
			pos = Vec3(2.f*tile.x + sin(rot)*(2.f - shift.y - 0.01f) + 2, 0.f, 2.f*tile.y + cos(rot)*(2.f - shift.y - 0.01f) + 2);
		else
			pos = Vec3(2.f*tile.x + sin(rot)*(2.f - shift.y - 0.01f), 0.f, 2.f*tile.y + cos(rot)*(2.f - shift.y - 0.01f));

		if(IS_SET(base->flags, OBJ_ON_WALL))
		{
			switch(dir)
			{
			case 0:
				pos.x += 1.f;
				break;
			case 1:
				pos.z += 1.f;
				break;
			case 2:
				pos.x -= 1.f;
				break;
			case 3:
				pos.z -= 1.f;
				break;
			}

			for(vector<Vec3>::iterator it2 = on_wall.begin(), end2 = on_wall.end(); it2 != end2; ++it2)
			{
				float dist = Vec3::Distance2d(*it2, pos);
				if(dist < 2.f)
					return nullptr;
			}
		}
		else
		{
			switch(dir)
			{
			case 0:
				pos.x += Random(0.2f, 1.8f);
				break;
			case 1:
				pos.z += Random(0.2f, 1.8f);
				break;
			case 2:
				pos.x -= Random(0.2f, 1.8f);
				break;
			case 3:
				pos.z -= Random(0.2f, 1.8f);
				break;
			}
		}
	}
	else if(IS_SET(base->flags, OBJ_IN_MIDDLE))
	{
		rot = PI / 2 * (Rand() % 4);
		pos = room.Center();
		switch(Rand() % 4)
		{
		case 0:
			pos.x += 2;
			break;
		case 1:
			pos.x -= 2;
			break;
		case 2:
			pos.z += 2;
			break;
		case 3:
			pos.z -= 2;
			break;
		}
	}
	else
	{
		rot = Random(MAX_ANGLE);
		float margin = (base->type == OBJ_CYLINDER ? base->r : max(base->size.x, base->size.y));
		if(!room.GetRandomPos(margin, pos))
			return nullptr;
	}

	if(IS_SET(base->flags, OBJ_HIGH))
		pos.y += 1.5f;

	if(base->type == OBJ_HITBOX)
	{
		// sprawd� kolizje z blokami
		if(!IS_SET(base->flags, OBJ_NO_PHYSICS))
		{
			if(NotZero(rot))
			{
				RotRect r1, r2;
				r1.center.x = pos.x;
				r1.center.y = pos.z;
				r1.rot = rot;
				r1.size = shift;
				r2.rot = 0;
				r2.size = Vec2(1, 1);
				for(vector<Int2>::iterator b_it = blocks.begin(), b_end = blocks.end(); b_it != b_end; ++b_it)
				{
					r2.center.x = 2.f*b_it->x + 1.f;
					r2.center.y = 2.f*b_it->y + 1.f;
					if(RotatedRectanglesCollision(r1, r2))
						return nullptr;
				}
			}
			else
			{
				for(vector<Int2>::iterator b_it = blocks.begin(), b_end = blocks.end(); b_it != b_end; ++b_it)
				{
					if(RectangleToRectangle(pos.x - shift.x, pos.z - shift.y, pos.x + shift.x, pos.z + shift.y,
						2.f*b_it->x, 2.f*b_it->y, 2.f*(b_it->x + 1), 2.f*(b_it->y + 1)))
						return nullptr;
				}
			}
		}

		// sprawd� kolizje z innymi obiektami
		Level::IgnoreObjects ignore = { 0 };
		ignore.ignore_blocks = true;
		L.global_col.clear();
		L.GatherCollisionObjects(L.local_ctx, L.global_col, pos, max(shift.x, shift.y) * SQRT_2, &ignore);
		if(!L.global_col.empty() && L.Collide(L.global_col, Box2d(pos.x - shift.x, pos.z - shift.y, pos.x + shift.x, pos.z + shift.y), 0.8f, rot))
			return nullptr;
	}
	else
	{
		// sprawd� kolizje z blokami
		if(!IS_SET(base->flags, OBJ_NO_PHYSICS))
		{
			for(vector<Int2>::iterator b_it = blocks.begin(), b_end = blocks.end(); b_it != b_end; ++b_it)
			{
				if(CircleToRectangle(pos.x, pos.z, shift.x, 2.f*b_it->x + 1.f, 2.f*b_it->y + 1.f, 1.f, 1.f))
					return nullptr;
			}
		}

		// sprawd� kolizje z innymi obiektami
		Level::IgnoreObjects ignore = { 0 };
		ignore.ignore_blocks = true;
		L.global_col.clear();
		L.GatherCollisionObjects(L.local_ctx, L.global_col, pos, base->r, &ignore);
		if(!L.global_col.empty() && L.Collide(L.global_col, pos, base->r + 0.8f))
			return nullptr;
	}

	if(IS_SET(base->flags, OBJ_ON_WALL))
		on_wall.push_back(pos);

	return L.SpawnObjectEntity(L.local_ctx, base, pos, rot, 1.f, flags);
}

//=================================================================================================
void InsideLocationGenerator::GenerateTraps()
{
	BaseLocation& base = g_base_locations[inside->target];

	if(!IS_SET(base.traps, TRAPS_NORMAL | TRAPS_MAGIC))
		return;

	InsideLocationLevel& lvl = GetLevelData();

	int szansa;
	Int2 pt(-1000, -1000);
	if(IS_SET(base.traps, TRAPS_NEAR_ENTRANCE))
	{
		if(dungeon_level != 0)
			return;
		szansa = 10;
		pt = lvl.staircase_up;
	}
	else if(IS_SET(base.traps, TRAPS_NEAR_END))
	{
		if(inside->IsMultilevel())
		{
			MultiInsideLocation* multi = (MultiInsideLocation*)inside;
			int size = int(multi->levels.size());
			switch(size)
			{
			case 0:
				szansa = 25;
				break;
			case 1:
				if(dungeon_level == 1)
					szansa = 25;
				else
					szansa = 0;
				break;
			case 2:
				if(dungeon_level == 2)
					szansa = 25;
				else if(dungeon_level == 1)
					szansa = 15;
				else
					szansa = 0;
				break;
			case 3:
				if(dungeon_level == 3)
					szansa = 25;
				else if(dungeon_level == 2)
					szansa = 15;
				else if(dungeon_level == 1)
					szansa = 10;
				else
					szansa = 0;
				break;
			default:
				if(dungeon_level == size - 1)
					szansa = 25;
				else if(dungeon_level == size - 2)
					szansa = 15;
				else if(dungeon_level == size - 3)
					szansa = 10;
				else
					szansa = 0;
				break;
			}

			if(szansa == 0)
				return;
		}
		else
			szansa = 20;
	}
	else
		szansa = 20;

	vector<TRAP_TYPE> traps;
	if(IS_SET(base.traps, TRAPS_NORMAL))
	{
		traps.push_back(TRAP_ARROW);
		traps.push_back(TRAP_POISON);
		traps.push_back(TRAP_SPEAR);
	}
	if(IS_SET(base.traps, TRAPS_MAGIC))
		traps.push_back(TRAP_FIREBALL);

	for(int y = 1; y < lvl.h - 1; ++y)
	{
		for(int x = 1; x < lvl.w - 1; ++x)
		{
			if(lvl.map[x + y * lvl.w].type == EMPTY
				&& !OR2_EQ(lvl.map[x - 1 + y * lvl.w].type, STAIRS_DOWN, STAIRS_UP)
				&& !OR2_EQ(lvl.map[x + 1 + y * lvl.w].type, STAIRS_DOWN, STAIRS_UP)
				&& !OR2_EQ(lvl.map[x + (y - 1)*lvl.w].type, STAIRS_DOWN, STAIRS_UP)
				&& !OR2_EQ(lvl.map[x + (y + 1)*lvl.w].type, STAIRS_DOWN, STAIRS_UP))
			{
				if(Rand() % 500 < szansa + max(0, 30 - Int2::Distance(pt, Int2(x, y))))
					L.CreateTrap(Int2(x, y), traps[Rand() % traps.size()]);
			}
		}
	}
}

//=================================================================================================
void InsideLocationGenerator::RegenerateTraps()
{
	Game& game = Game::Get();
	BaseLocation& base = g_base_locations[inside->target];

	if(!IS_SET(base.traps, TRAPS_MAGIC))
		return;

	InsideLocationLevel& lvl = GetLevelData();

	int szansa;
	Int2 pt(-1000, -1000);
	if(IS_SET(base.traps, TRAPS_NEAR_ENTRANCE))
	{
		if(dungeon_level != 0)
			return;
		szansa = 0;
		pt = lvl.staircase_up;
	}
	else if(IS_SET(base.traps, TRAPS_NEAR_END))
	{
		if(inside->IsMultilevel())
		{
			MultiInsideLocation* multi = (MultiInsideLocation*)inside;
			int size = int(multi->levels.size());
			switch(size)
			{
			case 0:
				szansa = 25;
				break;
			case 1:
				if(dungeon_level == 1)
					szansa = 25;
				else
					szansa = 0;
				break;
			case 2:
				if(dungeon_level == 2)
					szansa = 25;
				else if(dungeon_level == 1)
					szansa = 15;
				else
					szansa = 0;
				break;
			case 3:
				if(dungeon_level == 3)
					szansa = 25;
				else if(dungeon_level == 2)
					szansa = 15;
				else if(dungeon_level == 1)
					szansa = 10;
				else
					szansa = 0;
				break;
			default:
				if(dungeon_level == size - 1)
					szansa = 25;
				else if(dungeon_level == size - 2)
					szansa = 15;
				else if(dungeon_level == size - 3)
					szansa = 10;
				else
					szansa = 0;
				break;
			}

			if(szansa == 0)
				return;
		}
		else
			szansa = 20;
	}
	else
		szansa = 20;

	vector<Trap*>& traps = *L.local_ctx.traps;
	int id = 0, topid = traps.size();

	for(int y = 1; y < lvl.h - 1; ++y)
	{
		for(int x = 1; x < lvl.w - 1; ++x)
		{
			if(lvl.map[x + y * lvl.w].type == EMPTY
				&& !OR2_EQ(lvl.map[x - 1 + y * lvl.w].type, STAIRS_DOWN, STAIRS_UP)
				&& !OR2_EQ(lvl.map[x + 1 + y * lvl.w].type, STAIRS_DOWN, STAIRS_UP)
				&& !OR2_EQ(lvl.map[x + (y - 1)*lvl.w].type, STAIRS_DOWN, STAIRS_UP)
				&& !OR2_EQ(lvl.map[x + (y + 1)*lvl.w].type, STAIRS_DOWN, STAIRS_UP))
			{
				int s = szansa + max(0, 30 - Int2::Distance(pt, Int2(x, y)));
				if(IS_SET(base.traps, TRAPS_NORMAL))
					s /= 4;
				if(Rand() % 500 < s)
				{
					bool ok = false;
					if(id == -1)
						ok = true;
					else if(id == topid)
					{
						id = -1;
						ok = true;
					}
					else
					{
						while(id != topid)
						{
							if(traps[id]->tile.y > y || (traps[id]->tile.y == y && traps[id]->tile.x > x))
							{
								ok = true;
								break;
							}
							else if(traps[id]->tile.x == x && traps[id]->tile.y == y)
							{
								++id;
								break;
							}
							else
								++id;
						}
					}

					if(ok)
						L.CreateTrap(Int2(x, y), TRAP_FIREBALL);
				}
			}
		}
	}

	if(game.devmode)
		Info("Traps: %d", L.local_ctx.traps->size());
}

//=================================================================================================
void InsideLocationGenerator::RespawnTraps()
{
	for(vector<Trap*>::iterator it = L.local_ctx.traps->begin(), end = L.local_ctx.traps->end(); it != end; ++it)
	{
		Trap& trap = **it;

		trap.state = 0;
		if(trap.base->type == TRAP_SPEAR)
		{
			if(trap.hitted)
				trap.hitted->clear();
			else
				trap.hitted = new vector<Unit*>;
		}
	}
}

//=================================================================================================
void InsideLocationGenerator::CreateMinimap()
{
	Game& game = Game::Get();
	InsideLocationLevel& lvl = GetLevelData();
	TextureLock lock(game.tMinimap);

	for(int y = 0; y < lvl.h; ++y)
	{
		uint* pix = lock[y];
		for(int x = 0; x < lvl.w; ++x)
		{
			Tile& p = lvl.map[x + (lvl.w - 1 - y)*lvl.w];
			if(IS_SET(p.flags, Tile::F_REVEALED))
			{
				if(OR2_EQ(p.type, WALL, BLOCKADE_WALL))
					*pix = Color(100, 100, 100);
				else if(p.type == DOORS)
					*pix = Color(127, 51, 0);
				else
					*pix = Color(220, 220, 240);
			}
			else
				*pix = 0;
			++pix;
		}
	}

	// extra borders
	uint* pix = lock[lvl.h];
	for(int x = 0; x < lvl.w + 1; ++x)
	{
		*pix = 0;
		++pix;
	}
	for(int y = 0; y < lvl.h + 1; ++y)
	{
		uint* pix = lock[y] + lvl.w;
		*pix = 0;
	}

	L.minimap_size = lvl.w;
}

//=================================================================================================
void InsideLocationGenerator::OnLoad()
{
	Game& game = Game::Get();

	InsideLocation* inside = (InsideLocation*)loc;
	inside->SetActiveLevel(L.dungeon_level);
	BaseLocation& base = g_base_locations[inside->target];

	L.city_ctx = nullptr;
	L.ApplyContext(inside, L.local_ctx);
	game.SetDungeonParamsAndTextures(base);

	L.RecreateObjects(false);
	L.SpawnDungeonColliders();

	CreateMinimap();
}

//=================================================================================================
void InsideLocationGenerator::GenerateDungeonTreasure(vector<Chest*>& chests, int level, bool extra)
{
	int gold;
	if(chests.size() == 1)
	{
		vector<ItemSlot>& items = chests.front()->items;
		ItemHelper::GenerateTreasure(level, 10, items, gold, extra);
		InsertItemBare(items, Item::gold, (uint)gold, (uint)gold);
		SortItems(items);
	}
	else
	{
		static vector<ItemSlot> items;
		ItemHelper::GenerateTreasure(level, 9 + 2 * chests.size(), items, gold, extra);
		ItemHelper::SplitTreasure(items, gold, &chests[0], chests.size());
	}
}

//=================================================================================================
void InsideLocationGenerator::SpawnHeroesInsideDungeon()
{
	Game& game = Game::Get();
	InsideLocationLevel& lvl = GetLevelData();

	Room* p = lvl.GetUpStairsRoom();
	int room_id = lvl.GetRoomId(p);
	int chance = 23;
	bool first = true;

	vector<std::pair<Room*, int> > sprawdzone;
	vector<int> ok_room;
	sprawdzone.push_back(std::make_pair(p, room_id));

	while(true)
	{
		p = sprawdzone.back().first;
		for(vector<int>::iterator it = p->connected.begin(), end = p->connected.end(); it != end; ++it)
		{
			room_id = *it;
			bool ok = true;
			for(vector<std::pair<Room*, int> >::iterator it2 = sprawdzone.begin(), end2 = sprawdzone.end(); it2 != end2; ++it2)
			{
				if(room_id == it2->second)
				{
					ok = false;
					break;
				}
			}
			if(ok && (Rand() % 20 < chance || Rand() % 3 == 0 || first))
				ok_room.push_back(room_id);
		}

		first = false;

		if(ok_room.empty())
			break;
		else
		{
			room_id = ok_room[Rand() % ok_room.size()];
			ok_room.clear();
			sprawdzone.push_back(std::make_pair(&lvl.rooms[room_id], room_id));
			--chance;
		}
	}

	// cofnij ich z korytarza
	while(sprawdzone.back().first->IsCorridor())
		sprawdzone.pop_back();

	int gold = 0;
	vector<ItemSlot> items;
	LocalVector<Chest*> chests;

	// pozabijaj jednostki w pokojach, ograb skrzynie
	// troch� to nieefektywne :/
	vector<std::pair<Room*, int> >::iterator end = sprawdzone.end();
	if(Rand() % 2 == 0)
		--end;
	for(vector<Unit*>::iterator it2 = L.local_ctx.units->begin(), end2 = L.local_ctx.units->end(); it2 != end2; ++it2)
	{
		Unit& u = **it2;
		if(u.IsAlive() && game.pc->unit->IsEnemy(u))
		{
			for(vector<std::pair<Room*, int> >::iterator it = sprawdzone.begin(); it != end; ++it)
			{
				if(it->first->IsInside(u.pos))
				{
					gold += u.gold;
					for(int i = 0; i < SLOT_MAX; ++i)
					{
						if(u.slots[i] && u.slots[i]->GetWeightValue() >= 5.f)
						{
							ItemSlot& slot = Add1(items);
							slot.item = u.slots[i];
							slot.count = slot.team_count = 1u;
							u.weight -= u.slots[i]->weight;
							u.slots[i] = nullptr;
						}
					}
					for(vector<ItemSlot>::iterator it3 = u.items.begin(), end3 = u.items.end(); it3 != end3;)
					{
						if(it3->item->GetWeightValue() >= 5.f)
						{
							u.weight -= it3->item->weight * it3->count;
							items.push_back(*it3);
							it3 = u.items.erase(it3);
							end3 = u.items.end();
						}
						else
							++it3;
					}
					u.gold = 0;
					u.live_state = Unit::DEAD;
					if(u.data->mesh->IsLoaded())
					{
						u.animation = u.current_animation = ANI_DIE;
						u.SetAnimationAtEnd(NAMES::ani_die);
						L.CreateBlood(L.local_ctx, u, true);
					}
					else
						L.blood_to_spawn.push_back(&u);
					u.hp = 0.f;
					++GameStats::Get().total_kills;
					u.UpdatePhysics(Vec3::Zero);
					if(u.event_handler)
						u.event_handler->HandleUnitEvent(UnitEventHandler::DIE, &u);
					break;
				}
			}
		}
	}
	for(vector<Chest*>::iterator it2 = L.local_ctx.chests->begin(), end2 = L.local_ctx.chests->end(); it2 != end2; ++it2)
	{
		for(vector<std::pair<Room*, int> >::iterator it = sprawdzone.begin(); it != end; ++it)
		{
			if(it->first->IsInside((*it2)->pos))
			{
				for(vector<ItemSlot>::iterator it3 = (*it2)->items.begin(), end3 = (*it2)->items.end(); it3 != end3;)
				{
					if(it3->item->type == IT_GOLD)
					{
						gold += it3->count;
						it3 = (*it2)->items.erase(it3);
						end3 = (*it2)->items.end();
					}
					else if(it3->item->GetWeightValue() >= 5.f)
					{
						items.push_back(*it3);
						it3 = (*it2)->items.erase(it3);
						end3 = (*it2)->items.end();
					}
					else
						++it3;
				}
				chests->push_back(*it2);
				break;
			}
		}
	}

	// otw�rz drzwi pomi�dzy obszarami
	for(vector<std::pair<Room*, int> >::iterator it2 = sprawdzone.begin(), end2 = sprawdzone.end(); it2 != end2; ++it2)
	{
		Room& a = *it2->first,
			&b = lvl.rooms[it2->second];

		// wsp�lny obszar pomi�dzy pokojami
		int x1 = max(a.pos.x, b.pos.x),
			x2 = min(a.pos.x + a.size.x, b.pos.x + b.size.x),
			y1 = max(a.pos.y, b.pos.y),
			y2 = min(a.pos.y + a.size.y, b.pos.y + b.size.y);

		// szukaj drzwi
		for(int y = y1; y < y2; ++y)
		{
			for(int x = x1; x < x2; ++x)
			{
				Tile& po = lvl.map[x + y * lvl.w];
				if(po.type == DOORS)
				{
					Door* door = lvl.FindDoor(Int2(x, y));
					if(door && door->state == Door::Closed)
					{
						door->state = Door::Open;
						btVector3& pos = door->phy->getWorldTransform().getOrigin();
						pos.setY(pos.y() - 100.f);
						door->mesh_inst->SetToEnd(&door->mesh_inst->mesh->anims[0]);
					}
				}
			}
		}
	}

	// stw�rz bohater�w
	int count = Random(2, 4);
	LocalVector<Unit*> heroes;
	p = sprawdzone.back().first;
	int team_level = Random(4, 13);
	for(int i = 0; i < count; ++i)
	{
		int level = team_level + Random(-2, 2);
		Unit* u = L.SpawnUnitInsideRoom(*p, ClassInfo::GetRandomData(), level);
		if(u)
			heroes->push_back(u);
		else
			break;
	}

	// sortuj przedmioty wed�ug warto�ci
	std::sort(items.begin(), items.end(), [](const ItemSlot& a, const ItemSlot& b)
	{
		return a.item->GetWeightValue() < b.item->GetWeightValue();
	});

	// rozdziel z�oto
	int gold_per_hero = gold / heroes->size();
	for(vector<Unit*>::iterator it = heroes->begin(), end = heroes->end(); it != end; ++it)
		(*it)->gold += gold_per_hero;
	gold -= gold_per_hero * heroes->size();
	if(gold)
		heroes->back()->gold += gold;

	// rozdziel przedmioty
	vector<Unit*>::iterator heroes_it = heroes->begin(), heroes_end = heroes->end();
	while(!items.empty())
	{
		ItemSlot& item = items.back();
		for(int i = 0, count = item.count; i < count; ++i)
		{
			if((*heroes_it)->CanTake(item.item))
			{
				(*heroes_it)->AddItemAndEquipIfNone(item.item);
				--item.count;
				++heroes_it;
				if(heroes_it == heroes_end)
					heroes_it = heroes->begin();
			}
			else
			{
				// ten heros nie mo�e wzi��� tego przedmiotu, jest szansa �e wzi��by jaki� l�ejszy i ta�szy ale ma�a
				heroes_it = heroes->erase(heroes_it);
				if(heroes->empty())
					break;
				heroes_end = heroes->end();
				if(heroes_it == heroes_end)
					heroes_it = heroes->begin();
			}
		}
		if(heroes->empty())
			break;
		items.pop_back();
	}

	// pozosta�e przedmioty schowaj do skrzyni o ile jest co i gdzie
	if(!chests->empty() && !items.empty())
	{
		chests.Shuffle();
		vector<Chest*>::iterator chest_begin = chests->begin(), chest_end = chests->end(), chest_it = chest_begin;
		for(vector<ItemSlot>::iterator it = items.begin(), end = items.end(); it != end; ++it)
		{
			(*chest_it)->AddItem(it->item, it->count);
			++chest_it;
			if(chest_it == chest_end)
				chest_it = chest_begin;
		}
	}

	// sprawd� czy lokacja oczyszczona (raczej nie jest)
	L.CheckIfLocationCleared();
}
