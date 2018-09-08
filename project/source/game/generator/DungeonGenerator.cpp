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
int DungeonGenerator::GetNumberOfSteps()
{
	int steps = LocationGenerator::GetNumberOfSteps();
	if(first)
		steps += 2; // txGeneratingObjects, txGeneratingUnits
	else if(!reenter)
		++steps; // txRegeneratingLevel
	return steps;
}

//=================================================================================================
void DungeonGenerator::Generate()
{
	InsideLocation* inside = (InsideLocation*)loc;
	BaseLocation& base = g_base_locations[inside->target];
	InsideLocationLevel& lvl = inside->GetLevelData();

	// FIXME: dla EnterLevel
	inside->SetActiveLevel(0);
	if(inside->IsMultilevel())
	{
		MultiInsideLocation* multi = (MultiInsideLocation*)inside;
		if(multi->generated == 0)
			++multi->generated;
	}
	Info("Generating dungeon, target %d.", inside->target);

	assert(!lvl.map);

	OpcjeMapy opcje;
	opcje.korytarz_szansa = base.corridor_chance;
	opcje.w = opcje.h = base.size + base.size_lvl * dungeon_level;
	opcje.rozmiar_korytarz = base.corridor_size;
	opcje.rozmiar_pokoj = base.room_size;
	opcje.rooms = &lvl.rooms;
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
			Pole& p = opcje.mapa[pt.x + pt.y*opcje.w];
			p.type = DRZWI;
			p.flags |= Pole::F_SPECJALNE;

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
				lvl.map[r.pos.x + x + (r.pos.y + y)*lvl.w].flags |= Pole::F_DRUGA_TEKSTURA;
		}
	}

	// portal
	if(inside->from_portal)
	{
	powtorz:
		int id = Rand() % lvl.rooms.size();
		while(true)
		{
			Room& room = lvl.rooms[id];
			if(room.target != RoomTarget::None || room.size.x <= 4 || room.size.y <= 4)
				id = (id + 1) % lvl.rooms.size();
			else
				break;
		}

		Room& r = lvl.rooms[id];
		vector<std::pair<Int2, int> > good_pts;

		for(int y = 1; y < r.size.y - 1; ++y)
		{
			for(int x = 1; x < r.size.x - 1; ++x)
			{
				if(lvl.At(r.pos + Int2(x, y)).type == PUSTE)
				{
					// opcje:
					// ___ #__
					// _?_ #?_
					// ### ###
#define P(xx,yy) (lvl.At(r.pos+Int2(x+xx,y+yy)).type == PUSTE)
#define B(xx,yy) (lvl.At(r.pos+Int2(x+xx,y+yy)).type == SCIANA)

					int dir = -1;

					// __#
					// _?#
					// __#
					if(P(-1, 0) && B(1, 0) && B(1, -1) && B(1, 1) && ((P(-1, -1) && P(0, -1)) || (P(-1, 1) && P(0, 1)) || (B(-1, -1) && B(0, -1) && B(-1, 1) && B(0, 1))))
					{
						dir = 1;
					}
					// #__
					// #?_
					// #__
					else if(P(1, 0) && B(-1, 0) && B(-1, 1) && B(-1, -1) && ((P(0, -1) && P(1, -1)) || (P(0, 1) && P(1, 1)) || (B(0, -1) && B(1, -1) && B(0, 1) && B(1, 1))))
					{
						dir = 3;
					}
					// ###
					// _?_
					// ___
					else if(P(0, 1) && B(0, -1) && B(-1, -1) && B(1, -1) && ((P(-1, 0) && P(-1, 1)) || (P(1, 0) && P(1, 1)) || (B(-1, 0) && B(-1, 1) && B(1, 0) && B(1, 1))))
					{
						dir = 2;
					}
					// ___
					// _?_
					// ###
					else if(P(0, -1) && B(0, 1) && B(-1, 1) && B(1, 1) && ((P(-1, 0) && P(-1, -1)) || (P(1, 0) && P(1, -1)) || (B(-1, 0) && B(-1, -1) && B(1, 0) && B(1, -1))))
					{
						dir = 0;
					}

					if(dir != -1)
						good_pts.push_back(std::pair<Int2, int>(r.pos + Int2(x, y), dir));
#undef P
#undef B
				}
			}
		}

		if(good_pts.empty())
			goto powtorz;

		std::pair<Int2, int>& pt = good_pts[Rand() % good_pts.size()];

		const Vec3 pos(2.f*pt.first.x + 1, 0, 2.f*pt.first.y + 1);
		float rot = Clip(dir_to_rot(pt.second) + PI);

		inside->portal->pos = pos;
		inside->portal->rot = rot;

		lvl.GetRoom(pt.first)->target = RoomTarget::Portal;
	}
}

//=================================================================================================
void DungeonGenerator::GenerateObjects()
{
	Game& game = Game::Get();
	game.GenerateDungeonObjects2();
	game.GenerateDungeonObjects();
	GenerateTraps();
}

//=================================================================================================
void DungeonGenerator::GenerateUnits()
{
	Game& game = Game::Get();

	SPAWN_GROUP spawn_group;
	int base_level;

	if(L.location->spawn != SG_CHALLANGE)
	{
		spawn_group = L.location->spawn;
		base_level = game.GetDungeonLevel();
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
	UnitGroup& group = *spawn.unit_group;
	static vector<TmpUnitGroup> groups;

	int level = base_level;

	// poziomy 1..3
	for(int i = 0; i < 3; ++i)
	{
		TmpUnitGroup& part = Add1(groups);
		part.group = &group;
		part.Fill(level);
		level = min(base_level - i - 1, base_level * 4 / (i + 5));
	}

	// opcje wejœciowe (póki co tu)
	// musi byæ w sumie 100%
	int szansa_na_brak = 10,
		szansa_na_1 = 20,
		szansa_na_2 = 30,
		szansa_na_3 = 40,
		szansa_na_wrog_w_korytarz = 25;

	assert(InRange(szansa_na_brak, 0, 100) && InRange(szansa_na_1, 0, 100) && InRange(szansa_na_2, 0, 100) && InRange(szansa_na_3, 0, 100)
		&& InRange(szansa_na_wrog_w_korytarz, 0, 100) && szansa_na_brak + szansa_na_1 + szansa_na_2 + szansa_na_3 == 100);

	int szansa[3] = { szansa_na_brak, szansa_na_brak + szansa_na_1, szansa_na_brak + szansa_na_1 + szansa_na_2 };

	// dodaj jednostki
	InsideLocation* inside = (InsideLocation*)L.location;
	InsideLocationLevel& lvl = inside->GetLevelData();
	Int2 pt = lvl.staircase_up, pt2 = lvl.staircase_down;
	if(!inside->HaveDownStairs())
		pt2 = Int2(-1000, -1000);
	if(inside->from_portal)
		pt = pos_to_pt(inside->portal->pos);

	for(vector<Room>::iterator it = lvl.rooms.begin(), end = lvl.rooms.end(); it != end; ++it)
	{
		int ile;

		if(it->target == RoomTarget::Treasury || it->target == RoomTarget::Prison)
			continue;

		if(it->IsCorridor())
		{
			if(Rand() % 100 < szansa_na_wrog_w_korytarz)
				ile = 1;
			else
				continue;
		}
		else
		{
			int x = Rand() % 100;
			if(x < szansa[0])
				continue;
			else if(x < szansa[1])
				ile = 1;
			else if(x < szansa[2])
				ile = 2;
			else
				ile = 3;
		}

		TmpUnitGroup& part = groups[ile - 1];
		if(part.total == 0)
			continue;

		for(int i = 0; i < ile; ++i)
		{
			int x = Rand() % part.total,
				y = 0;

			for(auto& entry : part.entries)
			{
				y += entry.count;

				if(x < y)
				{
					// dodaj
					game.SpawnUnitInsideRoom(*it, *entry.ud, Random(part.max_level / 2, part.max_level), pt, pt2);
					break;
				}
			}
		}
	}

	// posprz¹taj
	groups.clear();
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
	Game& game = Game::Get();

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
	for(vector<Object*>::iterator it = game.local_ctx.objects->begin(), end = game.local_ctx.objects->end(); it != end; ++it)
	{
		Object& obj = **it;
		if(obj.base == table)
		{
			game.PickableItemBegin(game.local_ctx, obj);
			if(spawn_golden_cup)
			{
				spawn_golden_cup = false;
				game.PickableItemAdd(Item::Get("golden_cup"));
			}
			else
			{
				int count = Random(mod / 2, mod);
				if(count)
				{
					for(int i = 0; i < count; ++i)
						game.PickableItemAdd(lis.Get());
				}
			}
			if(Rand() % 3 == 0)
				game.PickableItemAdd(plate);
			if(Rand() % 3 == 0)
				game.PickableItemAdd(cup);
		}
		else if(obj.base == shelves)
		{
			int count = Random(mod, mod * 3 / 2);
			if(count)
			{
				game.PickableItemBegin(game.local_ctx, obj);
				for(int i = 0; i < count; ++i)
					game.PickableItemAdd(lis.Get());
			}
		}
	}
}
