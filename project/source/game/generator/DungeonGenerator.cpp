#include "Pch.h"
#include "GameCore.h"
#include "DungeonGenerator.h"
#include "InsideLocation.h"
#include "MultiInsideLocation.h"
#include "Mapa2.h"
#include "BaseLocation.h"
#include "Level.h"
#include "QuestManager.h"
#include "Quest_Secret.h"
#include "Quest_Evil.h"
#include "Quest_Orcs.h"
#include "Portal.h"
#include "UnitGroup.h"
#include "Game.h"

//=================================================================================================
void DungeonGenerator::Generate()
{
	InsideLocation* inside = (InsideLocation*)loc;
	BaseLocation& base = g_base_locations[inside->target];
	inside->SetActiveLevel(L.dungeon_level);
	if(inside->IsMultilevel())
	{
		MultiInsideLocation* multi = (MultiInsideLocation*)inside;
		if(multi->generated == 0)
			++multi->generated;
	}
	Info("Generating dungeon, target %d.", inside->target);

	InsideLocationLevel& lvl = inside->GetLevelData();
	assert(!lvl.map);

	OpcjeMapy opcje;
	opcje.korytarz_szansa = base.corridor_chance;
	opcje.w = opcje.h = base.size + base.size_lvl * dungeon_level;
	opcje.rozmiar_korytarz = base.corridor_size;
	opcje.rozmiar_pokoj = base.room_size;
	opcje.rooms = &lvl.rooms;
	opcje.groups = &lvl.groups;
	opcje.polacz_korytarz = base.join_corridor;
	opcje.polacz_pokoj = base.join_room;
	opcje.ksztalt = (IS_SET(base.options, BLO_ROUND) ? OpcjeMapy::OKRAG : OpcjeMapy::PROSTOKAT);
	opcje.schody_gora = (inside->HaveUpStairs() ? OpcjeMapy::LOSOWO : OpcjeMapy::BRAK);
	opcje.schody_dol = (inside->HaveDownStairs() ? OpcjeMapy::LOSOWO : OpcjeMapy::BRAK);
	opcje.kraty_szansa = base.bars_chance;
	opcje.devmode = Game::Get().devmode;

	// ostatni poziom krypty
	if(inside->type == L_CRYPT && !inside->HaveDownStairs())
	{
		opcje.schody_gora = OpcjeMapy::NAJDALEJ;
		Room& r = Add1(lvl.rooms);
		r.target = RoomTarget::Treasury;
		r.size = Int2(7, 7);
		r.pos.x = r.pos.y = (opcje.w - 7) / 2;
		inside->special_room = 0;
	}
	else if(inside->type == L_DUNGEON && (inside->target == THRONE_FORT || inside->target == THRONE_VAULT) && !inside->HaveDownStairs())
	{
		// sala tronowa
		opcje.schody_gora = OpcjeMapy::NAJDALEJ;
		Room& r = Add1(lvl.rooms);
		r.target = RoomTarget::Throne;
		r.size = Int2(13, 7);
		r.pos.x = (opcje.w - 13) / 2;
		r.pos.y = (opcje.w - 7) / 2;
		inside->special_room = 0;
	}
	else if(L.location_index == QM.quest_secret->where && QM.quest_secret->state == Quest_Secret::SECRET_DROPPED_STONE && !inside->HaveDownStairs())
	{
		// sekret
		opcje.schody_gora = OpcjeMapy::NAJDALEJ;
		Room& r = Add1(lvl.rooms);
		r.target = RoomTarget::PortalCreate;
		r.size = Int2(7, 7);
		r.pos.x = r.pos.y = (opcje.w - 7) / 2;
		inside->special_room = 0;
	}
	else if(L.location_index == QM.quest_evil->target_loc && QM.quest_evil->evil_state == Quest_Evil::State::GeneratedCleric)
	{
		// schody w krypcie 0 jak najdalej od œrodka
		opcje.schody_gora = OpcjeMapy::NAJDALEJ;
	}

	if(QM.quest_orcs2->orcs_state == Quest_Orcs2::State::Accepted && L.location_index == QM.quest_orcs->target_loc && dungeon_level == L.location->GetLastLevel())
	{
		opcje.stop = true;
		bool first = true;
		int proby = 0;
		vector<int> mozliwe_pokoje;

		while(true)
		{
			if(!generuj_mape2(opcje, !first))
			{
				assert(0);
				throw Format("Failed to generate dungeon map (%d)!", opcje.blad);
			}

			first = false;

			// szukaj pokoju dla celi
			int index = 0;
			for(vector<Room>::iterator it = lvl.rooms.begin(), end = lvl.rooms.end(); it != end; ++it, ++index)
			{
				if(it->target == RoomTarget::None && it->connected.size() == 1)
					mozliwe_pokoje.push_back(index);
			}

			if(mozliwe_pokoje.empty())
			{
				++proby;
				if(proby == 100)
					throw "Failed to generate special room in dungeon!";
				else
					continue;
			}

			int id = mozliwe_pokoje[Rand() % mozliwe_pokoje.size()];
			lvl.rooms[id].target = RoomTarget::Prison;
			// dodaj drzwi
			Int2 pt = pole_laczace(id, lvl.rooms[id].connected.front());
			Tile& p = opcje.mapa[pt.x + pt.y*opcje.w];
			p.type = DOORS;
			p.flags |= Tile::F_SPECIAL;

			if(!kontynuuj_generowanie_mapy(opcje))
			{
				assert(0);
				throw Format("Failed to generate dungeon map 2 (%d)!", opcje.blad);
			}

			break;
		}
	}
	else
	{
		if(!generuj_mape2(opcje))
		{
			assert(0);
			throw Format("Failed to generate dungeon map (%d)!", opcje.blad);
		}
	}

	lvl.w = lvl.h = opcje.w;
	lvl.map = opcje.mapa;
	lvl.staircase_up = opcje.schody_gora_pozycja;
	lvl.staircase_up_dir = opcje.schody_gora_kierunek;
	lvl.staircase_down = opcje.schody_dol_pozycja;
	lvl.staircase_down_dir = opcje.schody_dol_kierunek;
	lvl.staircase_down_in_wall = opcje.schody_dol_w_scianie;

	// inna tekstura pokoju w krypcie
	if(inside->type == L_CRYPT && !inside->HaveDownStairs())
	{
		Room& r = lvl.rooms[0];
		for(int y = 0; y < r.size.y; ++y)
		{
			for(int x = 0; x < r.size.x; ++x)
				lvl.map[r.pos.x + x + (r.pos.y + y)*lvl.w].flags |= Tile::F_SECOND_TEXTURE;
		}
	}

	if(inside->from_portal)
		CreatePortal(lvl);
}

//=================================================================================================
void DungeonGenerator::CreatePortal(InsideLocationLevel& lvl)
{
	vector<std::pair<Int2, GameDirection>> good_pts;

	for(int tries = 0; tries < 100; ++tries)
	{
		int group;
		Room& r = *lvl.GetRandomRoom(RoomTarget::None, [](Room& room) {return room.size.x >= 5 && room.size.y >= 5; }, nullptr, &group);

		for(int y = 1; y < r.size.y - 1; ++y)
		{
			for(int x = 1; x < r.size.x - 1; ++x)
			{
				if(lvl.At(r.pos + Int2(x, y)).type == EMPTY)
				{
					// opcje:
					// ___ #__
					// _?_ #?_
					// ### ###
#define P(xx,yy) (lvl.At(r.pos+Int2(x+xx,y+yy)).type == EMPTY)
#define B(xx,yy) (lvl.At(r.pos+Int2(x+xx,y+yy)).type == WALL)

					GameDirection dir = GDIR_INVALID;

					// __#
					// _?#
					// __#
					if(P(-1, 0) && B(1, 0) && B(1, -1) && B(1, 1) && ((P(-1, -1) && P(0, -1)) || (P(-1, 1) && P(0, 1)) || (B(-1, -1) && B(0, -1) && B(-1, 1) && B(0, 1))))
					{
						dir = GDIR_LEFT;
					}
					// #__
					// #?_
					// #__
					else if(P(1, 0) && B(-1, 0) && B(-1, 1) && B(-1, -1) && ((P(0, -1) && P(1, -1)) || (P(0, 1) && P(1, 1)) || (B(0, -1) && B(1, -1) && B(0, 1) && B(1, 1))))
					{
						dir = GDIR_RIGHT;
					}
					// ###
					// _?_
					// ___
					else if(P(0, 1) && B(0, -1) && B(-1, -1) && B(1, -1) && ((P(-1, 0) && P(-1, 1)) || (P(1, 0) && P(1, 1)) || (B(-1, 0) && B(-1, 1) && B(1, 0) && B(1, 1))))
					{
						dir = GDIR_UP;
					}
					// ___
					// _?_
					// ###
					else if(P(0, -1) && B(0, 1) && B(-1, 1) && B(1, 1) && ((P(-1, 0) && P(-1, -1)) || (P(1, 0) && P(1, -1)) || (B(-1, 0) && B(-1, -1) && B(1, 0) && B(1, -1))))
					{
						dir = GDIR_DOWN;
					}

					if(dir != GDIR_INVALID)
						good_pts.push_back(std::make_pair(r.pos + Int2(x, y), dir));
#undef P
#undef B
				}
			}
		}

		if(good_pts.empty())
			continue;

		std::pair<Int2, GameDirection>& pt = good_pts[Rand() % good_pts.size()];

		const Vec3 pos(2.f*pt.first.x + 1, 0, 2.f*pt.first.y + 1);
		float rot = Clip(DirToRot(pt.second) + PI);

		inside->portal->pos = pos;
		inside->portal->rot = rot;
		r.target = RoomTarget::Portal;
		lvl.groups[group].target = RoomTarget::Portal;
		break;
	}
}

//=================================================================================================
void DungeonGenerator::GenerateObjects()
{
	GenerateDungeonObjects();
	GenerateTraps();
}

//=================================================================================================
void DungeonGenerator::GenerateUnits()
{
	SPAWN_GROUP spawn_group;
	int base_level;

	if(L.location->spawn != SG_CHALLANGE)
	{
		spawn_group = L.location->spawn;
		base_level = L.GetDifficultyLevel();
	}
	else
	{
		base_level = L.location->st;
		if(dungeon_level == 0)
			spawn_group = SG_ORCS;
		else if(dungeon_level == 1)
			spawn_group = SG_MAGES_AND_GOLEMS;
		else
			spawn_group = SG_EVIL;
	}

	SpawnGroup& spawn = g_spawn_groups[spawn_group];
	if(!spawn.unit_group)
		return;
	Pooled<TmpUnitGroup> tmp;
	tmp->Fill(spawn.unit_group, base_level);

	// chance for spawning units
	const int chance_for_none = 10,
		chance_for_1 = 20,
		chance_for_2 = 30,
		chance_for_3 = 40,
		chance_in_corridor = 25;

	assert(InRange(chance_for_none, 0, 100) && InRange(chance_for_1, 0, 100) && InRange(chance_for_2, 0, 100) && InRange(chance_for_3, 0, 100)
		&& InRange(chance_in_corridor, 0, 100) && chance_for_none + chance_for_1 + chance_for_2 + chance_for_3 == 100);

	const int chance[3] = { chance_for_none, chance_for_none + chance_for_1, chance_for_none + chance_for_1 + chance_for_2 };

	// spawn units
	InsideLocation* inside = (InsideLocation*)L.location;
	InsideLocationLevel& lvl = inside->GetLevelData();
	Int2 down_stairs_pt = lvl.staircase_down;
	if(!inside->HaveDownStairs())
		down_stairs_pt = Int2(-1000, -1000);

	for(RoomGroup& group : lvl.groups)
	{
		if(Any(group.target, RoomTarget::Treasury, RoomTarget::Prison))
			continue;

		int count;
		if(group.target == RoomTarget::Corridor)
		{
			assert(group.rooms.size() == 1u);
			if(Rand() % 100 < chance_in_corridor)
				count = 1;
			else
				continue;
		}
		else
		{
			int x = Rand() % 100;
			if(x < chance[0])
				count = 0;
			else if(x < chance[1])
				count = 1;
			else if(x < chance[2])
				count = 2;
			else
				count = 3;
			count += group.rooms.size() - 1;
			if(count == 0)
				continue;
		}

		Int2 excluded_pt;
		if(group.target == RoomTarget::StairsUp)
			excluded_pt = lvl.staircase_up;
		else if(inside->from_portal && group.target == RoomTarget::Portal)
			excluded_pt = PosToPt(inside->portal->pos);
		else
			excluded_pt = Int2(-1000, -1000);

		for(TmpUnitGroup::Spawn& spawn : tmp->Roll(base_level, count))
		{
			Room& room = lvl.rooms[RandomItem(group.rooms)];
			L.SpawnUnitInsideRoom(room, *spawn.first, spawn.second, excluded_pt, down_stairs_pt);
		}
	}
}

//=================================================================================================
void DungeonGenerator::GenerateItems()
{
	GenerateDungeonItems();
}

//=================================================================================================
bool DungeonGenerator::HandleUpdate(int days)
{
	if(days >= 10)
		GenerateDungeonItems();
	return true;
}

//=================================================================================================
void DungeonGenerator::GenerateDungeonItems()
{
	// determine how much food to spawn
	int mod = 3;
	InsideLocation* inside = (InsideLocation*)loc;
	BaseLocation& base = g_base_locations[inside->target];

	if(IS_SET(base.options, BLO_LESS_FOOD))
		--mod;

	SPAWN_GROUP spawn = loc->spawn;
	if(spawn == SG_CHALLANGE)
	{
		if(dungeon_level == 0)
			spawn = SG_ORCS;
		else if(dungeon_level == 1)
			spawn = SG_MAGES_AND_GOLEMS;
		else
			spawn = SG_EVIL;
	}
	SpawnGroup& sg = g_spawn_groups[spawn];
	mod += sg.food_mod;

	if(mod <= 0)
		return;

	// get food list and base objects
	const ItemList& lis = *ItemList::Get(sg.orc_food ? "orc_food" : "normal_food").lis;
	BaseObject* table = BaseObject::Get("table"),
		*shelves = BaseObject::Get("shelves");
	const Item* plate = Item::Get("plate");
	const Item* cup = Item::Get("cup");
	bool spawn_golden_cup = Rand() % 100 == 0;

	// spawn food
	for(vector<Object*>::iterator it = L.local_ctx.objects->begin(), end = L.local_ctx.objects->end(); it != end; ++it)
	{
		Object& obj = **it;
		if(obj.base == table)
		{
			L.PickableItemBegin(L.local_ctx, obj);
			if(spawn_golden_cup)
			{
				spawn_golden_cup = false;
				L.PickableItemAdd(Item::Get("golden_cup"));
			}
			else
			{
				int count = Random(mod / 2, mod);
				if(count)
				{
					for(int i = 0; i < count; ++i)
						L.PickableItemAdd(lis.Get());
				}
			}
			if(Rand() % 3 == 0)
				L.PickableItemAdd(plate);
			if(Rand() % 3 == 0)
				L.PickableItemAdd(cup);
		}
		else if(obj.base == shelves)
		{
			int count = Random(mod, mod * 3 / 2);
			if(count)
			{
				L.PickableItemBegin(L.local_ctx, obj);
				for(int i = 0; i < count; ++i)
					L.PickableItemAdd(lis.Get());
			}
		}
	}
}
