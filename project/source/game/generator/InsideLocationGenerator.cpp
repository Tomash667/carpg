#include "Pch.h"
#include "GameCore.h"
#include "InsideLocationGenerator.h"
#include "InsideLocation.h"
#include "OutsideLocation.h"
#include "World.h"
#include "Level.h"
#include "QuestManager.h"
#include "Quest_Orcs.h"
#include "Quest_Secret.h"
#include "Stock.h"
#include "Debug.h"
#include "Portal.h"
#include "Game.h"

void InsideLocationGenerator::OnEnter()
{
	Game& game = Game::Get();
	InsideLocation* inside = (InsideLocation*)loc;
	inside->SetActiveLevel(dungeon_level);
	int days;
	bool need_reset = inside->CheckUpdate(days, W.GetWorldtime());
	InsideLocationLevel& lvl = inside->GetLevelData();
	BaseLocation& base = g_base_locations[inside->target];

	if(!reenter)
		game.ApplyContext(inside, game.local_ctx);

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
			game.UpdateLocation(game.local_ctx, days, base.door_open, need_reset);

		bool respawn_units = HandleUpdate(days);

		if(need_reset)
		{
			// usuñ ¿ywe jednostki
			if(days != 0)
			{
				for(vector<Unit*>::iterator it = game.local_ctx.units->begin(), end = game.local_ctx.units->end(); it != end; ++it)
				{
					if((*it)->IsAlive())
					{
						delete *it;
						*it = nullptr;
					}
				}
				RemoveNullElements(game.local_ctx.units);
			}

			// usuñ zawartoœæ skrzyni
			for(vector<Chest*>::iterator it = game.local_ctx.chests->begin(), end = game.local_ctx.chests->end(); it != end; ++it)
				(*it)->items.clear();

			// nowa zawartoœæ skrzyni
			int dlevel = game.GetDungeonLevel();
			for(vector<Chest*>::iterator it = game.local_ctx.chests->begin(), end = game.local_ctx.chests->end(); it != end; ++it)
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
			game.RegenerateTraps();
		}

		game.OnReenterLevel(game.local_ctx);

		// odtwórz jednostki
		if(respawn_units)
			game.RespawnUnits();
		game.RespawnTraps();

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
								Unit* u = game.SpawnUnitInsideRoom(*it, *ud, -2, Int2(-999, -999), Int2(-999, -999));
								if(u)
									u->dont_attack = true;
							}
						}

						Unit* u = game.SpawnUnitInsideRoom(lvl.GetFarRoom(false), *UnitData::Get("q_orkowie_kowal"), -2, Int2(-999, -999), Int2(-999, -999));
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
	game.CreateDungeonMinimap();

	// sekret
	Quest_Secret* secret = QM.quest_secret;
	if(L.location_index == secret->where && !inside->HaveDownStairs() && secret->state == Quest_Secret::SECRET_DROPPED_STONE)
	{
		secret->state = Quest_Secret::SECRET_GENERATED;
		if(game.devmode)
			Info("Generated secret room.");

		Room& r = inside->GetLevelData().rooms[0];

		if(game.hardcore_mode)
		{
			Object* o = game.local_ctx.FindObject(BaseObject::Get("portal"));

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
			Chest* c = game.local_ctx.FindChestInRoom(r);
			assert(c);
			if(c)
				c->AddItem(kartka, 1, 1);
			else
			{
				Object* o = game.local_ctx.FindObject(BaseObject::Get("portal"));
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
					game.local_ctx.items->push_back(item);
				}
				else
					game.SpawnGroundItemInsideRoom(r, kartka);
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

InsideLocationLevel& InsideLocationGenerator::GetLevelData()
{
	InsideLocation* inside = (InsideLocation*)loc;
	return inside->GetLevelData();
}
