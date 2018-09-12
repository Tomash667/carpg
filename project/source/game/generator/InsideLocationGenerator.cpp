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

		// FIXME - add step for generating items
		GenerateItems();
	}
	else if(!reenter)
	{
		game.LoadingStep(game.txRegeneratingLevel);

		if(days > 0)
			game.UpdateLocation(L.local_ctx, days, base.door_open, need_reset);

		bool respawn_units = HandleUpdate(days);

		if(need_reset)
		{
			// usuñ ¿ywe jednostki
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

			// usuñ zawartoœæ skrzyni
			for(vector<Chest*>::iterator it = L.local_ctx.chests->begin(), end = L.local_ctx.chests->end(); it != end; ++it)
				(*it)->items.clear();

			// nowa zawartoœæ skrzyni
			int dlevel = game.GetDungeonLevel();
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
				game.GenerateDungeonTreasure(room_chests, dlevel);
				room_chests.clear();
			}

			// nowe jednorazowe pu³apki
			RegenerateTraps();
		}

		game.OnReenterLevel(L.local_ctx);

		// odtwórz jednostki
		if(respawn_units)
			RespawnUnits();
		RespawnTraps();

		// odtwórz fizykê
		game.RespawnObjectColliders();

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

					// generowanie orków
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
		game.SpawnHeroesInsideDungeon();

	// stwórz obiekty kolizji
	if(!reenter)
		game.SpawnDungeonColliders();

	// generuj minimapê
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
			// dodaj kartkê (overkill sprawdzania!)
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

	// dodaj gracza i jego dru¿ynê
	Int2 spawn_pt;
	Vec3 spawn_pos;
	float spawn_rot;

	if(L.enter_from < ENTER_FROM_PORTAL)
	{
		if(L.enter_from == ENTER_FROM_DOWN_LEVEL)
		{
			spawn_pt = lvl.GetDownStairsFrontTile();
			spawn_rot = dir_to_rot(lvl.staircase_down_dir);
		}
		else
		{
			spawn_pt = lvl.GetUpStairsFrontTile();
			spawn_rot = dir_to_rot(lvl.staircase_up_dir);
		}
		spawn_pos = pt_to_pos(spawn_pt);
	}
	else
	{
		Portal* portal = inside->GetPortal(L.enter_from);
		spawn_pos = portal->GetSpawnPos();
		spawn_rot = Clip(portal->rot + PI);
		spawn_pt = pos_to_pt(spawn_pos);
	}

	game.AddPlayerTeam(spawn_pos, spawn_rot, reenter, L.enter_from == ENTER_FROM_OUTSIDE);
	game.OpenDoorsByTeam(spawn_pt);
}

//=================================================================================================
InsideLocationLevel& InsideLocationGenerator::GetLevelData()
{
	return inside->GetLevelData();
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
			if(lvl.map[x + y * lvl.w].type == PUSTE
				&& !OR2_EQ(lvl.map[x - 1 + y * lvl.w].type, SCHODY_DOL, SCHODY_GORA)
				&& !OR2_EQ(lvl.map[x + 1 + y * lvl.w].type, SCHODY_DOL, SCHODY_GORA)
				&& !OR2_EQ(lvl.map[x + (y - 1)*lvl.w].type, SCHODY_DOL, SCHODY_GORA)
				&& !OR2_EQ(lvl.map[x + (y + 1)*lvl.w].type, SCHODY_DOL, SCHODY_GORA))
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
			if(lvl.map[x + y * lvl.w].type == PUSTE
				&& !OR2_EQ(lvl.map[x - 1 + y * lvl.w].type, SCHODY_DOL, SCHODY_GORA)
				&& !OR2_EQ(lvl.map[x + 1 + y * lvl.w].type, SCHODY_DOL, SCHODY_GORA)
				&& !OR2_EQ(lvl.map[x + (y - 1)*lvl.w].type, SCHODY_DOL, SCHODY_GORA)
				&& !OR2_EQ(lvl.map[x + (y + 1)*lvl.w].type, SCHODY_DOL, SCHODY_GORA))
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
			Pole& p = lvl.map[x + (lvl.w - 1 - y)*lvl.w];
			if(IS_SET(p.flags, Pole::F_ODKRYTE))
			{
				if(OR2_EQ(p.type, SCIANA, BLOKADA_SCIANA))
					*pix = Color(100, 100, 100);
				else if(p.type == DRZWI)
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

	game.minimap_size = lvl.w;
}

void InsideLocationGenerator::OnLoad()
{
	Game& game = Game::Get();

	InsideLocation* inside = (InsideLocation*)loc;
	inside->SetActiveLevel(L.dungeon_level);
	BaseLocation& base = g_base_locations[inside->target];

	L.city_ctx = nullptr;
	L.ApplyContext(inside, L.local_ctx);
	game.SetDungeonParamsAndTextures(base);

	game.RespawnObjectColliders(false);
	game.SpawnDungeonColliders();

	CreateMinimap();
}
