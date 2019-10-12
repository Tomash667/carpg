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
#include "Portal.h"
#include "Texture.h"
#include "GameStats.h"
#include "RoomType.h"
#include "ItemHelper.h"
#include "Game.h"
#include "Team.h"
#include "Pathfinding.h"
#include "GameResources.h"

// don't spawn objects near other objects to not block path
const float EXTRA_RADIUS = 0.8f;

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
	inside->SetActiveLevel(dungeon_level);
	int days;
	bool need_reset = inside->CheckUpdate(days, world->GetWorldtime());
	InsideLocationLevel& lvl = inside->GetLevelData();
	BaseLocation& base = g_base_locations[inside->target];

	if(!reenter)
		game_level->Apply();

	game->SetDungeonParamsAndTextures(base);

	if(first)
	{
		game->LoadingStep(game->txGeneratingObjects);
		GenerateObjects();

		game->LoadingStep(game->txGeneratingUnits);
		GenerateUnits();

		GenerateItems();
	}
	else if(!reenter)
	{
		game->LoadingStep(game->txRegeneratingLevel);

		if(days > 0)
			game_level->UpdateLocation(days, base.door_open, need_reset);

		int update_flags = HandleUpdate(days);
		if(IsSet(update_flags, PREVENT_RESET))
			need_reset = false;

		if(need_reset)
		{
			// usuñ ¿ywe jednostki
			if(days != 0)
				DeleteElements(lvl.units, [](Unit* unit) { return unit->IsAlive(); });

			// usuñ zawartoœæ skrzyni
			for(vector<Chest*>::iterator it = lvl.chests.begin(), end = lvl.chests.end(); it != end; ++it)
				(*it)->items.clear();

			// nowa zawartoœæ skrzyni
			int dlevel = game_level->GetDifficultyLevel();
			for(vector<Chest*>::iterator it = lvl.chests.begin(), end = lvl.chests.end(); it != end; ++it)
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

			// nowe jednorazowe pu³apki
			RegenerateTraps();
		}

		game_level->OnReenterLevel();

		// odtwórz jednostki
		if(!IsSet(update_flags, PREVENT_RESPAWN_UNITS))
			RespawnUnits();
		RespawnTraps();

		// odtwórz fizykê
		if(!IsSet(update_flags, PREVENT_RECREATE_OBJECTS))
			game_level->RecreateObjects();

		if(need_reset)
			GenerateUnits();
	}
	else
		OnReenter();

	// questowe rzeczy
	if(inside->active_quest && inside->active_quest != ACTIVE_QUEST_HOLDER && inside->active_quest->type != Q_SCRIPTED)
	{
		Quest_Event* event = inside->active_quest->GetEvent(game_level->location_index);
		if(event)
		{
			if(event->at_level == dungeon_level)
			{
				if(!event->done)
				{
					quest_mgr->HandleQuestEvent(event);

					// generowanie orków
					if(game_level->location_index == quest_mgr->quest_orcs2->target_loc && quest_mgr->quest_orcs2->orcs_state == Quest_Orcs2::State::GenerateOrcs)
					{
						quest_mgr->quest_orcs2->orcs_state = Quest_Orcs2::State::GeneratedOrcs;
						UnitData* ud = UnitData::Get("q_orkowie_slaby");
						for(Room* room : lvl.rooms)
						{
							if(!room->IsCorridor() && Rand() % 2 == 0)
							{
								Unit* u = game_level->SpawnUnitInsideRoom(*room, *ud, -2, Int2(-999, -999), Int2(-999, -999));
								if(u)
									u->dont_attack = true;
							}
						}

						UnitData* orc_smith = UnitData::Get("q_orkowie_kowal");
						Unit* u = game_level->SpawnUnitInsideRoom(lvl.GetFarRoom(false), *orc_smith, -2, Int2(-999, -999), Int2(-999, -999));
						if(u)
							u->dont_attack = true;
					}
				}

				game_level->event_handler = event->location_event_handler;
			}
			else if(inside->active_quest->whole_location_event_handler)
				game_level->event_handler = event->location_event_handler;
		}
	}

	if((first || need_reset) && (Rand() % 50 == 0 || GKey.DebugKey(Key::C)) && game_level->location->type != L_CAVE && inside->target != LABYRINTH
		&& !game_level->location->active_quest && dungeon_level == 0 && !game_level->location->group->IsEmpty() && inside->IsMultilevel())
		SpawnHeroesInsideDungeon();

	// stwórz obiekty kolizji
	if(!reenter)
		game_level->SpawnDungeonColliders();

	// generuj minimapê
	game->LoadingStep(game->txGeneratingMinimap);
	CreateMinimap();

	// sekret
	Quest_Secret* secret = quest_mgr->quest_secret;
	if(game_level->location_index == secret->where && !inside->HaveDownStairs() && secret->state == Quest_Secret::SECRET_DROPPED_STONE)
	{
		secret->state = Quest_Secret::SECRET_GENERATED;
		if(game->devmode)
			Info("Generated secret room.");

		Room& r = *GetLevelData().rooms[0];

		if(game->hardcore_mode)
		{
			Object* o = lvl.FindObject(BaseObject::Get("portal"));

			OutsideLocation* loc = new OutsideLocation;
			loc->active_quest = ACTIVE_QUEST_HOLDER;
			loc->pos = Vec2(-999, -999);
			loc->st = 20;
			loc->name = game->txHiddenPlace;
			loc->type = L_OUTSIDE;
			loc->image = LI_FOREST;
			loc->state = LS_HIDDEN;
			int loc_id = world->AddLocation(loc);

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
			// dodaj kartkê (overkill sprawdzania!)
			const Item* kartka = Item::Get("sekret_kartka2");
			assert(kartka);
			Chest* c = lvl.FindChestInRoom(r);
			assert(c);
			if(c)
				c->AddItem(kartka, 1, 1);
			else
			{
				Object* o = lvl.FindObject(BaseObject::Get("portal"));
				assert(0);
				if(o)
				{
					GroundItem* item = new GroundItem;
					item->Register();
					item->count = 1;
					item->team_count = 1;
					item->item = kartka;
					item->pos = o->pos;
					item->rot = Random(MAX_ANGLE);
					lvl.items.push_back(item);
				}
				else
					game_level->SpawnGroundItemInsideRoom(r, kartka);
			}

			secret->state = Quest_Secret::SECRET_CLOSED;
		}
	}

	// dodaj gracza i jego dru¿ynê
	Int2 spawn_pt;
	Vec3 spawn_pos;
	float spawn_rot;

	if(game_level->enter_from < ENTER_FROM_PORTAL)
	{
		if(game_level->enter_from == ENTER_FROM_DOWN_LEVEL)
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
		Portal* portal = inside->GetPortal(game_level->enter_from);
		spawn_pos = portal->GetSpawnPos();
		spawn_rot = Clip(portal->rot + PI);
		spawn_pt = PosToPt(spawn_pos);
	}

	game_level->AddPlayerTeam(spawn_pos, spawn_rot, reenter, game_level->enter_from == ENTER_FROM_OUTSIDE);
	OpenDoorsByTeam(spawn_pt);
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
		TILE_TYPE type = lvl.map[room.pos.x + x + (room.pos.y + room.size.y - 1)*lvl.w].type;
		if(Any(type, EMPTY, BARS, BARS_FLOOR, BARS_CEILING, DOORS, HOLE_FOR_DOORS))
		{
			blocks.push_back(Int2(room.pos.x + x, room.pos.y + room.size.y - 1));
			blocks.push_back(Int2(room.pos.x + x, room.pos.y + room.size.y - 2));
		}
		else if(type == WALL || type == BLOCKADE_WALL)
			blocks.push_back(Int2(room.pos.x + x, room.pos.y + room.size.y - 1));

		// bottom
		type = lvl.map[room.pos.x + x + room.pos.y*lvl.w].type;
		if(Any(type, EMPTY, BARS, BARS_FLOOR, BARS_CEILING, DOORS, HOLE_FOR_DOORS))
		{
			blocks.push_back(Int2(room.pos.x + x, room.pos.y));
			blocks.push_back(Int2(room.pos.x + x, room.pos.y + 1));
		}
		else if(type == WALL || type == BLOCKADE_WALL)
			blocks.push_back(Int2(room.pos.x + x, room.pos.y));
	}
	for(int y = 0; y < room.size.y; ++y)
	{
		// left
		TILE_TYPE type = lvl.map[room.pos.x + (room.pos.y + y)*lvl.w].type;
		if(Any(type, EMPTY, BARS, BARS_FLOOR, BARS_CEILING, DOORS, HOLE_FOR_DOORS))
		{
			blocks.push_back(Int2(room.pos.x, room.pos.y + y));
			blocks.push_back(Int2(room.pos.x + 1, room.pos.y + y));
		}
		else if(type == WALL || type == BLOCKADE_WALL)
			blocks.push_back(Int2(room.pos.x, room.pos.y + y));

		// right
		type = lvl.map[room.pos.x + room.size.x - 1 + (room.pos.y + y)*lvl.w].type;
		if(Any(type, EMPTY, BARS, BARS_FLOOR, BARS_CEILING, DOORS, HOLE_FOR_DOORS))
		{
			blocks.push_back(Int2(room.pos.x + room.size.x - 1, room.pos.y + y));
			blocks.push_back(Int2(room.pos.x + room.size.x - 2, room.pos.y + y));
		}
		else if(type == WALL || type == BLOCKADE_WALL)
			blocks.push_back(Int2(room.pos.x + room.size.x - 1, room.pos.y + y));
	}
}

//=================================================================================================
void InsideLocationGenerator::GenerateDungeonObjects()
{
	InsideLocationLevel& lvl = GetLevelData();
	BaseLocation& base = g_base_locations[inside->target];
	static vector<Chest*> room_chests;
	static vector<Vec3> on_wall;
	static vector<Int2> blocks;
	int chest_lvl = game_level->GetChestDifficultyLevel();

	// schody w górê
	if(inside->HaveUpStairs())
	{
		Object* o = new Object;
		o->mesh = game_res->aStairsUp;
		o->pos = PtToPos(lvl.staircase_up);
		o->rot = Vec3(0, DirToRot(lvl.staircase_up_dir), 0);
		o->scale = 1;
		o->base = nullptr;
		lvl.objects.push_back(o);
	}
	else
		game_level->SpawnObjectEntity(lvl, BaseObject::Get("portal"), inside->portal->pos, inside->portal->rot);

	// schody w dó³
	if(inside->HaveDownStairs())
	{
		Object* o = new Object;
		o->mesh = (lvl.staircase_down_in_wall ? game_res->aStairsDown2 : game_res->aStairsDown);
		o->pos = PtToPos(lvl.staircase_down);
		o->rot = Vec3(0, DirToRot(lvl.staircase_down_dir), 0);
		o->scale = 1;
		o->base = nullptr;
		lvl.objects.push_back(o);
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
				o->mesh = game_res->aGrating;
				o->rot = Vec3(0, 0, 0);
				o->pos = Vec3(float(x * 2), 0, float(y * 2));
				o->scale = 1;
				o->base = nullptr;
				lvl.objects.push_back(o);
			}
			if(p == BARS || p == BARS_CEILING)
			{
				Object* o = new Object;
				o->mesh = game_res->aGrating;
				o->rot = Vec3(0, 0, 0);
				o->pos = Vec3(float(x * 2), 4, float(y * 2));
				o->scale = 1;
				o->base = nullptr;
				lvl.objects.push_back(o);
			}
			if(p == DOORS)
			{
				Object* o = new Object;
				o->mesh = game_res->aDoorWall;
				if(IsSet(lvl.map[x + y * lvl.w].flags, Tile::F_SECOND_TEXTURE))
					o->mesh = game_res->aDoorWall2;
				o->pos = Vec3(float(x * 2) + 1, 0, float(y * 2) + 1);
				o->scale = 1;
				o->base = nullptr;
				lvl.objects.push_back(o);

				if(IsBlocking(lvl.map[x - 1 + y * lvl.w].type))
				{
					o->rot = Vec3(0, 0, 0);
					int mov = 0;
					if(lvl.rooms[lvl.map[x + (y - 1)*lvl.w].room]->IsCorridor())
						++mov;
					if(lvl.rooms[lvl.map[x + (y + 1)*lvl.w].room]->IsCorridor())
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
					if(lvl.rooms[lvl.map[x - 1 + y * lvl.w].room]->IsCorridor())
						++mov;
					if(lvl.rooms[lvl.map[x + 1 + y * lvl.w].room]->IsCorridor())
						--mov;
					if(mov == 1)
						o->pos.x += 0.8229f;
					else if(mov == -1)
						o->pos.x -= 0.8229f;
				}

				if(Rand() % 100 < base.door_chance || IsSet(lvl.map[x + y * lvl.w].flags, Tile::F_SPECIAL))
				{
					Door* door = new Door;
					door->Register();
					lvl.doors.push_back(door);
					door->pt = Int2(x, y);
					door->pos = o->pos;
					door->rot = o->rot.y;
					door->state = Door::Closed;
					door->mesh_inst = new MeshInstance(game_res->aDoor);
					door->mesh_inst->groups[0].speed = 2.f;
					door->phy = new btCollisionObject;
					door->phy->setCollisionShape(game_level->shape_door);
					door->phy->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT | CG_DOOR);
					door->locked = LOCK_NONE;
					btTransform& tr = door->phy->getWorldTransform();
					Vec3 pos = door->pos;
					pos.y += Door::HEIGHT;
					tr.setOrigin(ToVector3(pos));
					tr.setRotation(btQuaternion(door->rot, 0, 0));
					phy_world->addCollisionObject(door->phy, CG_DOOR);

					if(IsSet(lvl.map[x + y * lvl.w].flags, Tile::F_SPECIAL))
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
	if(IsSet(base.options, BLO_MAGIC_LIGHT))
		flags = Level::SOE_MAGIC_LIGHT;

	bool required = false;
	if(base.required.value)
		required = true;

	for(Room* room : lvl.rooms)
	{
		if(room->IsCorridor())
			continue;

		AddRoomColliders(lvl, *room, blocks);

		// choose room type
		RoomType* rt;
		if(room->target != RoomTarget::None)
		{
			if(room->target == RoomTarget::Treasury)
				rt = RoomType::Find("krypta_skarb");
			else if(room->target == RoomTarget::Throne)
				rt = RoomType::Find("tron");
			else if(room->target == RoomTarget::PortalCreate)
				rt = RoomType::Find("portal");
			else
			{
				Int2 pt;
				if(room->target == RoomTarget::StairsDown)
					pt = lvl.staircase_down;
				else if(room->target == RoomTarget::StairsUp)
					pt = lvl.staircase_up;
				else if(room->target == RoomTarget::Portal)
				{
					if(inside->portal)
						pt = PosToPt(inside->portal->pos);
					else
					{
						Object* o = lvl.FindObject(BaseObject::Get("portal"));
						if(o)
							pt = PosToPt(o->pos);
						else
							pt = room->CenterTile();
					}
				}

				for(int y = -1; y <= 1; ++y)
				{
					for(int x = -1; x <= 1; ++x)
						blocks.push_back(Int2(pt.x + x, pt.y + y));
				}

				if(base.stairs.value)
					rt = base.stairs.value;
				else
					rt = base.GetRandomRoomType();
			}
		}
		else
		{
			if(required)
				rt = base.required.value;
			else
				rt = base.GetRandomRoomType();
		}

		int fail = 10;
		bool required_object = false;

		// try to spawn all objects
		for(uint i = 0; i < rt->count && fail > 0; ++i)
		{
			bool is_group = false;
			BaseObject* base = BaseObject::Get(rt->objs[i].id, &is_group);
			int count = rt->objs[i].count.Random();

			for(int j = 0; j < count && fail > 0; ++j)
			{
				ObjectEntity e = GenerateDungeonObject(lvl, *room, base, on_wall, blocks, flags);
				if(!e)
				{
					if(IsSet(base->flags, OBJ_IMPORTANT))
						--j;
					--fail;
					continue;
				}

				if(e.type == ObjectEntity::CHEST)
					room_chests.push_back(e);

				if(IsSet(base->flags, OBJ_REQUIRED))
					required_object = true;

				if(is_group)
					base = BaseObject::Get(rt->objs[i].id);
			}
		}

		if(required && required_object && room->target == RoomTarget::None)
			required = false;

		if(!room_chests.empty())
		{
			bool extra = IsSet(rt->flags, RT_TREASURE);
			GenerateDungeonTreasure(room_chests, chest_lvl, extra);
			room_chests.clear();
		}

		on_wall.clear();
		blocks.clear();
	}

	if(required)
		throw "Failed to generate required object!";

	if(lvl.chests.empty())
	{
		// niech w podziemiach bêdzie minimum 1 skrzynia
		BaseObject* base = BaseObject::Get("chest");
		for(int i = 0; i < 100; ++i)
		{
			on_wall.clear();
			blocks.clear();
			Room& r = *lvl.rooms[Rand() % lvl.rooms.size()];
			if(r.target == RoomTarget::None)
			{
				AddRoomColliders(lvl, r, blocks);

				ObjectEntity e = GenerateDungeonObject(lvl, r, base, on_wall, blocks, flags);
				if(e)
				{
					GenerateDungeonTreasure(lvl.chests, chest_lvl);
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

	if(IsSet(base->flags, OBJ_NEAR_WALL))
	{
		Int2 tile;
		GameDirection dir;
		if(!lvl.GetRandomNearWallTile(room, tile, dir, IsSet(base->flags, OBJ_ON_WALL)))
			return nullptr;

		rot = DirToRot(dir);
		if(dir == 2 || dir == 3)
			pos = Vec3(2.f*tile.x + sin(rot)*(2.f - shift.y - 0.01f) + 2, 0.f, 2.f*tile.y + cos(rot)*(2.f - shift.y - 0.01f) + 2);
		else
			pos = Vec3(2.f*tile.x + sin(rot)*(2.f - shift.y - 0.01f), 0.f, 2.f*tile.y + cos(rot)*(2.f - shift.y - 0.01f));

		if(IsSet(base->flags, OBJ_ON_WALL))
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
	else if(IsSet(base->flags, OBJ_IN_MIDDLE))
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

	if(IsSet(base->flags, OBJ_HIGH))
		pos.y += 1.5f;

	if(base->type == OBJ_HITBOX)
	{
		// sprawdŸ kolizje z blokami
		if(!IsSet(base->flags, OBJ_NO_PHYSICS))
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

		// sprawdŸ kolizje z innymi obiektami
		Level::IgnoreObjects ignore = { 0 };
		ignore.ignore_blocks = true;
		game_level->global_col.clear();
		game_level->GatherCollisionObjects(lvl, game_level->global_col, pos, max(shift.x, shift.y) * SQRT_2, &ignore);
		if(!game_level->global_col.empty() && game_level->Collide(game_level->global_col, Box2d(pos.x - shift.x, pos.z - shift.y, pos.x + shift.x, pos.z + shift.y), EXTRA_RADIUS, rot))
			return nullptr;
	}
	else
	{
		// sprawdŸ kolizje z blokami
		if(!IsSet(base->flags, OBJ_NO_PHYSICS))
		{
			for(vector<Int2>::iterator b_it = blocks.begin(), b_end = blocks.end(); b_it != b_end; ++b_it)
			{
				if(CircleToRectangle(pos.x, pos.z, shift.x, 2.f*b_it->x + 1.f, 2.f*b_it->y + 1.f, 1.f, 1.f))
					return nullptr;
			}
		}

		// sprawdŸ kolizje z innymi obiektami
		Level::IgnoreObjects ignore = { 0 };
		ignore.ignore_blocks = true;
		game_level->global_col.clear();
		game_level->GatherCollisionObjects(lvl, game_level->global_col, pos, base->r, &ignore);
		if(!game_level->global_col.empty() && game_level->Collide(game_level->global_col, pos, base->r + EXTRA_RADIUS))
			return nullptr;
	}

	if(IsSet(base->flags, OBJ_ON_WALL))
		on_wall.push_back(pos);

	return game_level->SpawnObjectEntity(lvl, base, pos, rot, 1.f, flags);
}

//=================================================================================================
ObjectEntity InsideLocationGenerator::GenerateDungeonObject(InsideLocationLevel& lvl, const Int2& tile, BaseObject* base)
{
	assert(base);
	assert(base->type == OBJ_CYLINDER); // not implemented

	Box2d allowed_region(2.f * tile.x, 2.f * tile.y, 2.f * (tile.x + 1), 2.f * (tile.y + 1));
	int dir_flags = lvl.GetTileDirFlags(tile);
	if(IsSet(dir_flags, GDIRF_LEFT_ROW))
		allowed_region.v1.x += base->r;
	if(IsSet(dir_flags, GDIRF_RIGHT_ROW))
		allowed_region.v2.x -= base->r;
	if(IsSet(dir_flags, GDIRF_DOWN_ROW))
		allowed_region.v1.y += base->r;
	if(IsSet(dir_flags, GDIRF_UP_ROW))
		allowed_region.v2.y -= base->r;

	if(allowed_region.SizeX() < base->r + EXTRA_RADIUS
		|| allowed_region.SizeY() < base->r + EXTRA_RADIUS)
		return nullptr;

	float rot = Random(MAX_ANGLE);
	Vec3 pos = allowed_region.GetRandomPos3();
	Level::IgnoreObjects ignore = { 0 };
	ignore.ignore_blocks = true;
	game_level->global_col.clear();
	game_level->GatherCollisionObjects(lvl, game_level->global_col, pos, base->r + EXTRA_RADIUS, &ignore);
	if(!game_level->global_col.empty() && game_level->Collide(game_level->global_col, pos, base->r + EXTRA_RADIUS))
		return nullptr;

	return game_level->SpawnObjectEntity(lvl, base, pos, rot, 1.f, 0);
}

//=================================================================================================
void InsideLocationGenerator::GenerateTraps()
{
	BaseLocation& base = g_base_locations[inside->target];

	if(!IsSet(base.traps, TRAPS_NORMAL | TRAPS_MAGIC))
		return;

	InsideLocationLevel& lvl = GetLevelData();

	int szansa;
	Int2 pt(-1000, -1000);
	if(IsSet(base.traps, TRAPS_NEAR_ENTRANCE))
	{
		if(dungeon_level != 0)
			return;
		szansa = 10;
		pt = lvl.staircase_up;
	}
	else if(IsSet(base.traps, TRAPS_NEAR_END))
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
	if(IsSet(base.traps, TRAPS_NORMAL))
	{
		traps.push_back(TRAP_ARROW);
		traps.push_back(TRAP_POISON);
		traps.push_back(TRAP_SPEAR);
	}
	if(IsSet(base.traps, TRAPS_MAGIC))
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
					game_level->CreateTrap(Int2(x, y), traps[Rand() % traps.size()]);
			}
		}
	}
}

//=================================================================================================
void InsideLocationGenerator::RegenerateTraps()
{
	BaseLocation& base = g_base_locations[inside->target];

	if(!IsSet(base.traps, TRAPS_MAGIC))
		return;

	InsideLocationLevel& lvl = GetLevelData();

	int szansa;
	Int2 pt(-1000, -1000);
	if(IsSet(base.traps, TRAPS_NEAR_ENTRANCE))
	{
		if(dungeon_level != 0)
			return;
		szansa = 0;
		pt = lvl.staircase_up;
	}
	else if(IsSet(base.traps, TRAPS_NEAR_END))
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

	vector<Trap*>& traps = lvl.traps;
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
				if(IsSet(base.traps, TRAPS_NORMAL))
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
						game_level->CreateTrap(Int2(x, y), TRAP_FIREBALL);
				}
			}
		}
	}

	if(game->devmode)
		Info("Traps: %d", traps.size());
}

//=================================================================================================
void InsideLocationGenerator::RespawnTraps()
{
	for(Trap* trap : game_level->local_area->traps)
	{
		trap->state = 0;
		if(trap->base->type == TRAP_SPEAR)
		{
			if(trap->hitted)
				trap->hitted->clear();
			else
				trap->hitted = new vector<Unit*>;
		}
	}
}

//=================================================================================================
void InsideLocationGenerator::CreateMinimap()
{
	InsideLocationLevel& lvl = GetLevelData();
	TextureLock lock(game->tMinimap.tex);

	for(int y = 0; y < lvl.h; ++y)
	{
		uint* pix = lock[y];
		for(int x = 0; x < lvl.w; ++x)
		{
			Tile& p = lvl.map[x + (lvl.w - 1 - y)*lvl.w];
			if(IsSet(p.flags, Tile::F_REVEALED))
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

	game_level->minimap_size = lvl.w;
}

//=================================================================================================
void InsideLocationGenerator::OnLoad()
{
	InsideLocation* inside = (InsideLocation*)loc;
	inside->SetActiveLevel(game_level->dungeon_level);
	BaseLocation& base = g_base_locations[inside->target];

	game->SetDungeonParamsAndTextures(base);
	game_level->RecreateObjects(Net::IsClient());
	game_level->SpawnDungeonColliders();
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
	InsideLocationLevel& lvl = GetLevelData();

	vector<RoomGroup*> groups;
	FindPathFromStairsToStairs(groups);

	uint depth = (Random(groups.size() / 3, groups.size() - 1u) + Random(groups.size() / 3, groups.size() - 1u)) / 2;
	while(groups[depth]->target == RoomTarget::Corridor)
		--depth; // don't stay in corridor

	vector<Room*> visited;
	for(uint i = 0; i < depth; ++i)
	{
		RoomGroup* group = groups[i];
		for(int index : group->rooms)
			visited.push_back(lvl.rooms[index]);
	}

	int gold = 0;
	vector<ItemSlot> items;
	LocalVector<Chest*> chests;

	// kill units in visited rooms
	for(vector<Unit*>::iterator it2 = lvl.units.begin(), end2 = lvl.units.end(); it2 != end2; ++it2)
	{
		Unit& u = **it2;
		if(u.IsAlive() && game->pc->unit->IsEnemy(u))
		{
			for(Room* room : visited)
			{
				if(!room->IsInside(u.pos))
					continue;

				gold += u.gold;
				for(int i = 0; i < SLOT_MAX; ++i)
				{
					if(u.slots[i] && u.slots[i]->GetWeightValue() >= 5.f)
					{
						ItemSlot& slot = Add1(items);
						slot.item = u.slots[i];
						slot.count = slot.team_count = 1u;
						u.weight -= u.slots[i]->weight;
						u.RemoveItemEffects(u.slots[i], (ITEM_SLOT)i);
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
					game_level->CreateBlood(lvl, u, true);
				}
				else
					game_level->blood_to_spawn.push_back(&u);
				u.hp = 0.f;
				++game_stats->total_kills;
				u.UpdatePhysics();
				if(u.event_handler)
					u.event_handler->HandleUnitEvent(UnitEventHandler::DIE, &u);
				break;
			}
		}
	}

	// loot chests in visited rooms
	for(vector<Chest*>::iterator it2 = lvl.chests.begin(), end2 = lvl.chests.end(); it2 != end2; ++it2)
	{
		for(Room* room : visited)
		{
			if(!room->IsInside((*it2)->pos))
				continue;

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

	// open doors between rooms
	for(uint i = 0; i < depth; ++i)
	{
		pair<Room*, Room*> connected = lvl.GetConnectingRooms(groups[i], groups[i + 1]);
		Room& a = *connected.first,
			&b = *connected.second;

		// get common area between rooms
		int x1 = max(a.pos.x, b.pos.x),
			x2 = min(a.pos.x + a.size.x, b.pos.x + b.size.x),
			y1 = max(a.pos.y, b.pos.y),
			y2 = min(a.pos.y + a.size.y, b.pos.y + b.size.y);

		// find doors
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

	// create heroes
	int count = Random(3, 4);
	LocalVector<Unit*> heroes;
	Room* room;
	RoomGroup* group = groups[depth];
	if(group->rooms.size() == 1u)
		room = lvl.rooms[group->rooms.front()];
	else
		room = lvl.GetConnectingRooms(groups[depth - 1], group).second;
	for(int i = 0; i < count; ++i)
	{
		int level = loc->st + Random(-2, 2);
		Unit* u = game_level->SpawnUnitInsideRoom(*room, *Class::GetRandomHeroData(), level);
		if(u)
			heroes->push_back(u);
		else
			break;
	}

	// split gold
	int gold_per_hero = gold / heroes->size();
	for(vector<Unit*>::iterator it = heroes->begin(), end = heroes->end(); it != end; ++it)
		(*it)->gold += gold_per_hero;
	gold -= gold_per_hero * heroes->size();
	if(gold)
		heroes->back()->gold += gold;

	// split items
	std::sort(items.begin(), items.end(), [](const ItemSlot& a, const ItemSlot& b)
	{
		return a.item->GetWeightValue() < b.item->GetWeightValue();
	});
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
				// this hero can't carry more items
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

	// put rest of items inside some chest
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

	// check if location is cleared (realy small chance)
	game_level->CheckIfLocationCleared();
}

//=================================================================================================
void InsideLocationGenerator::FindPathFromStairsToStairs(vector<RoomGroup*>& groups)
{
	InsideLocationLevel& lvl = GetLevelData();

	vector<Int2> path;
	pathfinding->FindPath(lvl, lvl.staircase_up, lvl.staircase_down, path);
	std::reverse(path.begin(), path.end());

	RoomGroup* prev_group = nullptr;
	word prev_room_index = 0xFFFF;
	for(const Int2& pt : path)
	{
		word room_index = lvl.map[pt.x + pt.y * lvl.w].room;
		if(room_index != prev_room_index)
		{
			RoomGroup* group = &lvl.groups[lvl.rooms[room_index]->group];
			if(group != prev_group)
			{
				groups.push_back(group);
				prev_group = group;
			}
			prev_room_index = room_index;
		}
	}
}

//=================================================================================================
void InsideLocationGenerator::OpenDoorsByTeam(const Int2& pt)
{
	static vector<Int2> tmp_path;
	InsideLocationLevel& lvl = inside->GetLevelData();
	for(Unit& unit : team->members)
	{
		Int2 unit_pt = PosToPt(unit.pos);
		if(pathfinding->FindPath(lvl, unit_pt, pt, tmp_path))
		{
			for(vector<Int2>::iterator it2 = tmp_path.begin(), end2 = tmp_path.end(); it2 != end2; ++it2)
			{
				Tile& p = lvl.map[(*it2)(lvl.w)];
				if(p.type == DOORS)
				{
					Door* door = lvl.FindDoor(*it2);
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
		else
			Warn("OpenDoorsByTeam: Can't find path from unit %s (%d,%d) to spawn point (%d,%d).", unit.data->id.c_str(), unit_pt.x, unit_pt.y, pt.x, pt.y);
	}
}
