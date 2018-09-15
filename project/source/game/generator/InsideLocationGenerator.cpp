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
			L.UpdateLocation(days, base.door_open, need_reset);

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
		SpawnHeroesInsideDungeon();

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

	game.RespawnObjectColliders(false);
	game.SpawnDungeonColliders();

	CreateMinimap();
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
	// trochê to nieefektywne :/
	vector<std::pair<Room*, int> >::iterator end = sprawdzone.end();
	if(Rand() % 2 == 0)
		--end;
	for(vector<Unit*>::iterator it2 = L.local_ctx.units->begin(), end2 = L.local_ctx.units->end(); it2 != end2; ++it2)
	{
		Unit& u = **it2;
		if(u.IsAlive() && game.IsEnemy(*game.pc->unit, u))
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

					// przenieœ fizyke
					btVector3 a_min, a_max;
					u.cobj->getWorldTransform().setOrigin(btVector3(1000, 1000, 1000));
					u.cobj->getCollisionShape()->getAabb(u.cobj->getWorldTransform(), a_min, a_max);
					game.phy_broadphase->setAabb(u.cobj->getBroadphaseHandle(), a_min, a_max, game.phy_dispatcher);

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

	// otwórz drzwi pomiêdzy obszarami
	for(vector<std::pair<Room*, int> >::iterator it2 = sprawdzone.begin(), end2 = sprawdzone.end(); it2 != end2; ++it2)
	{
		Room& a = *it2->first,
			&b = lvl.rooms[it2->second];

		// wspólny obszar pomiêdzy pokojami
		int x1 = max(a.pos.x, b.pos.x),
			x2 = min(a.pos.x + a.size.x, b.pos.x + b.size.x),
			y1 = max(a.pos.y, b.pos.y),
			y2 = min(a.pos.y + a.size.y, b.pos.y + b.size.y);

		// szukaj drzwi
		for(int y = y1; y < y2; ++y)
		{
			for(int x = x1; x < x2; ++x)
			{
				Pole& po = lvl.map[x + y * lvl.w];
				if(po.type == DRZWI)
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

	// stwórz bohaterów
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

	// sortuj przedmioty wed³ug wartoœci
	std::sort(items.begin(), items.end(), [](const ItemSlot& a, const ItemSlot& b)
	{
		return a.item->GetWeightValue() < b.item->GetWeightValue();
	});

	// rozdziel z³oto
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
		for(int i = 0, ile = item.count; i < ile; ++i)
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
				// ten heros nie mo¿e wzi¹œæ tego przedmiotu, jest szansa ¿e wzi¹³by jakiœ l¿ejszy i tañszy ale ma³a
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

	// pozosta³e przedmioty schowaj do skrzyni o ile jest co i gdzie
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

	// sprawdŸ czy lokacja oczyszczona (raczej nie jest)
	game.CheckIfLocationCleared();
}
