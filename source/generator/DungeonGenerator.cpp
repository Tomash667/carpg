#include "Pch.h"
#include "DungeonGenerator.h"
#include "InsideLocation.h"
#include "MultiInsideLocation.h"
#include "DungeonMapGenerator.h"
#include "BaseLocation.h"
#include "Level.h"
#include "QuestManager.h"
#include "Quest_Secret.h"
#include "Quest_Evil.h"
#include "Quest_Orcs.h"
#include "Quest_Scripted.h"
#include "Portal.h"
#include "UnitGroup.h"
#include "Game.h"
#include "ScriptManager.h"

//=================================================================================================
void DungeonGenerator::Generate()
{
	InsideLocation* inside = (InsideLocation*)loc;
	BaseLocation& base = g_base_locations[inside->target];
	inside->SetActiveLevel(game_level->dungeon_level);
	game_level->lvl = &inside->GetLevelData();
	if(inside->IsMultilevel())
	{
		MultiInsideLocation* multi = (MultiInsideLocation*)inside;
		if(multi->generated == 0)
			++multi->generated;
	}
	Info("Generating dungeon, target %d.", inside->target);

	InsideLocationLevel& lvl = inside->GetLevelData();
	assert(!lvl.map);

	Quest_Scripted* event_handler = nullptr;
	for(Event& event : inside->events)
	{
		if(event.type == EVENT_GENERATE)
		{
			event_handler = event.quest;
			break;
		}
	}

	MapSettings settings;
	settings.corridor_chance = base.corridor_chance;
	settings.map_w = settings.map_h = base.size + base.size_lvl * dungeon_level;
	settings.corridor_size = base.corridor_size;
	settings.room_size = base.room_size;
	settings.rooms = &lvl.rooms;
	settings.groups = &lvl.groups;
	settings.corridor_join_chance = base.join_corridor;
	settings.room_join_chance = base.join_room;
	settings.shape = (IsSet(base.options, BLO_ROUND) ? MapSettings::SHAPE_CIRCLE : MapSettings::SHAPE_SQUARE);
	settings.stairs_up_loc = (inside->HaveUpStairs() ? MapSettings::STAIRS_RANDOM : MapSettings::STAIRS_NONE);
	settings.stairs_down_loc = (inside->HaveDownStairs() ? MapSettings::STAIRS_RANDOM : MapSettings::STAIRS_NONE);
	settings.bars_chance = base.bars_chance;
	settings.devmode = game->devmode;
	settings.remove_dead_end_corridors = true;

	bool skip_normal_handling = false;
	if(event_handler)
	{
		ScriptEvent e(EVENT_GENERATE);
		e.on_generate.location = inside;
		e.on_generate.map_settings = &settings;
		e.on_generate.stage = 0;
		event_handler->FireEvent(e);
		skip_normal_handling = e.cancel;
	}

	if(skip_normal_handling)
	{
	}
	else if(Any(inside->target, HERO_CRYPT, MONSTER_CRYPT) && !inside->HaveDownStairs())
	{
		// last crypt level
		Room* room = Room::Get();
		room->target = RoomTarget::Treasury;
		room->type = nullptr;
		room->size = Int2(7, 7);
		room->pos.x = room->pos.y = (settings.map_w - 7) / 2;
		room->connected.clear();
		room->index = 0;
		inside->special_room = 0;
		lvl.rooms.push_back(room);
		settings.stairs_up_loc = MapSettings::STAIRS_FAR_FROM_ROOM;
	}
	else if(inside->type == L_DUNGEON && (inside->target == THRONE_FORT || inside->target == THRONE_VAULT) && !inside->HaveDownStairs())
	{
		// throne room
		Room* room = Room::Get();
		room->target = RoomTarget::Throne;
		room->type = nullptr;
		room->size = Int2(13, 7);
		room->pos.x = (settings.map_w - 13) / 2;
		room->pos.y = (settings.map_w - 7) / 2;
		room->connected.clear();
		room->index = 0;
		inside->special_room = 0;
		lvl.rooms.push_back(room);
		settings.stairs_up_loc = MapSettings::STAIRS_FAR_FROM_ROOM;
	}
	else if(game_level->location_index == quest_mgr->quest_secret->where && quest_mgr->quest_secret->state == Quest_Secret::SECRET_DROPPED_STONE && !inside->HaveDownStairs())
	{
		// secret
		Room* room = Room::Get();
		room->target = RoomTarget::PortalCreate;
		room->type = nullptr;
		room->size = Int2(7, 7);
		room->pos.x = room->pos.y = (settings.map_w - 7) / 2;
		room->connected.clear();
		room->index = 0;
		inside->special_room = 0;
		lvl.rooms.push_back(room);
		settings.stairs_up_loc = MapSettings::STAIRS_FAR_FROM_ROOM;
	}
	else if(game_level->location_index == quest_mgr->quest_evil->target_loc && quest_mgr->quest_evil->evil_state == Quest_Evil::State::GeneratedCleric)
	{
		// schody w krypcie 0 jak najdalej od œrodka
		settings.stairs_up_loc = MapSettings::STAIRS_FAR_FROM_ROOM;
	}

	if(quest_mgr->quest_orcs2->orcs_state == Quest_Orcs2::State::Accepted && game_level->location_index == quest_mgr->quest_orcs->target_loc && dungeon_level == game_level->location->GetLastLevel())
	{
		// search for room for cell
		settings.stop = true;
		bool first = true;
		int tries = 0;
		vector<Room*> possible_rooms;

		while(true)
		{
			map_gen.Generate(settings, !first);
			first = false;

			for(Room* room : lvl.rooms)
			{
				if(room->target == RoomTarget::None && room->connected.size() == 1)
					possible_rooms.push_back(room);
			}

			if(possible_rooms.empty())
			{
				++tries;
				if(tries == 100)
					throw "Failed to generate special room in dungeon!";
				else
					continue;
			}

			Room* room = RandomItem(possible_rooms);
			room->target = RoomTarget::Prison;
			Int2 pt = map_gen.GetConnectingTile(room, room->connected.front());
			Tile& p = map_gen.GetMap()[pt.x + pt.y * settings.map_w];
			p.type = DOORS;
			p.flags |= Tile::F_SPECIAL;

			map_gen.ContinueGenerating();
			break;
		}
	}
	else
		map_gen.Generate(settings);

	lvl.w = lvl.h = settings.map_w;
	lvl.map = map_gen.GetMap();
	lvl.staircase_up = settings.stairs_up_pos;
	lvl.staircase_up_dir = settings.stairs_up_dir;
	lvl.staircase_down = settings.stairs_down_pos;
	lvl.staircase_down_dir = settings.stairs_down_dir;
	lvl.staircase_down_in_wall = settings.stairs_down_in_wall;

	// different texture in crypt treasure room
	if(!skip_normal_handling && Any(inside->target, HERO_CRYPT, MONSTER_CRYPT) && !inside->HaveDownStairs())
	{
		Room& r = *lvl.rooms[0];
		for(int y = 0; y < r.size.y; ++y)
		{
			for(int x = 0; x < r.size.x; ++x)
				lvl.map[r.pos.x + x + (r.pos.y + y) * lvl.w].flags |= Tile::F_SECOND_TEXTURE;
		}
	}

	if(event_handler)
	{
		ScriptEvent e(EVENT_GENERATE);
		e.on_generate.location = inside;
		e.on_generate.map_settings = nullptr;
		e.on_generate.stage = 1;
		event_handler->FireEvent(e);
	}

	if(inside->from_portal)
		CreatePortal(lvl);
}

//=================================================================================================
void DungeonGenerator::CreatePortal(InsideLocationLevel& lvl)
{
	vector<pair<Int2, GameDirection>> good_pts;

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

		pair<Int2, GameDirection>& pt = good_pts[Rand() % good_pts.size()];

		const Vec3 pos(2.f * pt.first.x + 1, 0, 2.f * pt.first.y + 1);
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
	if(game_level->location->group->IsEmpty())
		return;

	UnitGroup* group = GetGroup();
	int base_level;
	if(game_level->location->group->IsChallange())
		base_level = game_level->location->st; // all levels have max st
	else
		base_level = game_level->GetDifficultyLevel();

	Pooled<TmpUnitGroup> tmp;
	tmp->Fill(group, base_level);

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
	InsideLocation* inside = (InsideLocation*)game_level->location;
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
			Room& room = *lvl.rooms[RandomItem(group.rooms)];
			game_level->SpawnUnitInsideRoom(room, *spawn.first, spawn.second, excluded_pt, down_stairs_pt);
		}
	}
}

//=================================================================================================
void DungeonGenerator::GenerateItems()
{
	GenerateDungeonItems();
}

//=================================================================================================
int DungeonGenerator::HandleUpdate(int days)
{
	if(days >= 10)
		GenerateDungeonItems();
	return 0;
}

//=================================================================================================
void DungeonGenerator::GenerateDungeonItems()
{
	// determine how much food to spawn
	int mod = 3;
	InsideLocation* inside = (InsideLocation*)loc;
	BaseLocation& base = g_base_locations[inside->target];

	if(IsSet(base.options, BLO_LESS_FOOD))
		--mod;

	UnitGroup* group = GetGroup();
	mod += group->food_mod;

	if(mod <= 0)
		return;

	// get food list and base objects
	const ItemList& lis = *ItemList::Get(group->orc_food ? "orc_food" : "normal_food").lis;
	BaseObject* table = BaseObject::Get("table"),
		*shelves = BaseObject::Get("shelves");
	const Item* plate = Item::Get("plate");
	const Item* cup = Item::Get("cup");
	bool spawn_golden_cup = Rand() % 100 == 0;

	// spawn food
	LevelArea& area = *game_level->local_area;
	for(vector<Object*>::iterator it = area.objects.begin(), end = area.objects.end(); it != end; ++it)
	{
		Object& obj = **it;
		if(obj.base == table)
		{
			game_level->PickableItemBegin(area, obj);
			if(spawn_golden_cup)
			{
				spawn_golden_cup = false;
				game_level->PickableItemAdd(Item::Get("golden_cup"));
			}
			else
			{
				int count = Random(mod / 2, mod);
				if(count)
				{
					for(int i = 0; i < count; ++i)
						game_level->PickableItemAdd(lis.Get());
				}
			}
			if(Rand() % 3 == 0)
				game_level->PickableItemAdd(plate);
			if(Rand() % 3 == 0)
				game_level->PickableItemAdd(cup);
		}
		else if(obj.base == shelves)
		{
			int count = Random(mod, mod * 3 / 2);
			if(count)
			{
				game_level->PickableItemBegin(area, obj);
				for(int i = 0; i < count; ++i)
					game_level->PickableItemAdd(lis.Get());
			}
		}
	}
}

//=================================================================================================
UnitGroup* DungeonGenerator::GetGroup()
{
	UnitGroup* group = loc->group;
	if(group->IsChallange())
	{
		assert(group->is_list);
		group = group->entries[dungeon_level % group->entries.size()].group;
	}
	return group;
}
