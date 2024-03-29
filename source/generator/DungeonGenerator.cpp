#include "Pch.h"
#include "DungeonGenerator.h"

#include "BaseLocation.h"
#include "DungeonMapGenerator.h"
#include "Game.h"
#include "InsideLocation.h"
#include "Level.h"
#include "MultiInsideLocation.h"
#include "Portal.h"
#include "QuestManager.h"
#include "Quest_Evil.h"
#include "Quest_Orcs.h"
#include "Quest_Secret.h"
#include "Quest_Scripted.h"
#include "UnitGroup.h"

//=================================================================================================
void DungeonGenerator::Generate()
{
	InsideLocation* inside = (InsideLocation*)loc;
	BaseLocation& base = gBaseLocations[inside->target];
	inside->SetActiveLevel(gameLevel->dungeonLevel);
	gameLevel->lvl = &inside->GetLevelData();
	if(inside->IsMultilevel())
	{
		MultiInsideLocation* multi = (MultiInsideLocation*)inside;
		if(multi->generated == 0)
			++multi->generated;
	}
	Info("Generating dungeon, target %d.", inside->target);

	InsideLocationLevel& lvl = inside->GetLevelData();
	assert(!lvl.map);

	Quest2* eventHandler = nullptr;
	for(Event& event : inside->events)
	{
		if(event.type == EVENT_GENERATE)
		{
			eventHandler = event.quest;
			break;
		}
	}

	MapSettings settings;
	settings.corridorChance = base.corridorChance;
	settings.mapW = settings.mapH = base.size + base.sizeLvl * dungeonLevel;
	settings.corridorSize = base.corridorSize;
	settings.roomSize = base.roomSize;
	settings.rooms = &lvl.rooms;
	settings.groups = &lvl.groups;
	settings.corridorJoinChance = base.joinCorridor;
	settings.roomJoinChance = base.joinRoom;
	settings.shape = IsSet(base.options, BLO_ROUND) ? MapSettings::SHAPE_CIRCLE : MapSettings::SHAPE_SQUARE;
	if(gameLevel->dungeonLevel == 0)
	{
		if(IsSet(base.options, BLO_GOES_UP))
		{
			settings.prevEntryType = IsSet(base.options, BLO_DOOR_ENTRY) ? ENTRY_DOOR : ENTRY_STAIRS_DOWN;
			settings.nextEntryType = ENTRY_STAIRS_UP;
		}
		else
		{
			settings.prevEntryType = IsSet(base.options, BLO_DOOR_ENTRY) ? ENTRY_DOOR : ENTRY_STAIRS_UP;
			settings.nextEntryType = ENTRY_STAIRS_DOWN;
		}
	}
	else
	{
		if(static_cast<MultiInsideLocation*>(inside)->levels[0]->nextEntryType == ENTRY_STAIRS_UP)
		{
			settings.prevEntryType = ENTRY_STAIRS_DOWN;
			settings.nextEntryType = ENTRY_STAIRS_UP;
		}
		else
		{
			settings.prevEntryType = ENTRY_STAIRS_UP;
			settings.nextEntryType = ENTRY_STAIRS_DOWN;
		}
	}
	settings.prevEntryLoc = inside->HavePrevEntry() ? MapSettings::ENTRY_RANDOM : MapSettings::ENTRY_NONE;
	if(settings.prevEntryLoc == MapSettings::ENTRY_RANDOM && settings.prevEntryType == ENTRY_DOOR)
		settings.prevEntryLoc = MapSettings::ENTRY_BORDER;
	settings.nextEntryLoc = inside->HaveNextEntry() ? MapSettings::ENTRY_RANDOM : MapSettings::ENTRY_NONE;
	settings.barsChance = base.barsChance;
	settings.devmode = game->devmode;
	settings.removeDeadEndCorridors = true;

	bool skipNormalHandling = false;
	if(eventHandler)
	{
		ScriptEvent e(EVENT_GENERATE);
		e.onGenerate.location = inside;
		e.onGenerate.mapSettings = &settings;
		e.onGenerate.stage = 0;
		e.onGenerate.cancel = false;
		eventHandler->FireEvent(e);
		skipNormalHandling = e.onGenerate.cancel;
	}

	if(skipNormalHandling)
	{
	}
	else if(Any(inside->target, HERO_CRYPT, MONSTER_CRYPT) && !inside->HaveNextEntry())
	{
		// last crypt level
		Room* room = Room::Get();
		room->target = RoomTarget::Treasury;
		room->type = nullptr;
		room->size = Int2(7, 7);
		room->pos.x = room->pos.y = (settings.mapW - 7) / 2;
		room->connected.clear();
		room->index = 0;
		inside->specialRoom = 0;
		lvl.rooms.push_back(room);
		settings.prevEntryLoc = MapSettings::ENTRY_FAR_FROM_ROOM;
	}
	else if(inside->type == L_DUNGEON && (inside->target == THRONE_FORT || inside->target == THRONE_VAULT) && !inside->HaveNextEntry())
	{
		// throne room
		Room* room = Room::Get();
		room->target = RoomTarget::Throne;
		room->type = nullptr;
		room->size = Int2(13, 7);
		room->pos.x = (settings.mapW - 13) / 2;
		room->pos.y = (settings.mapW - 7) / 2;
		room->connected.clear();
		room->index = 0;
		inside->specialRoom = 0;
		lvl.rooms.push_back(room);
		settings.prevEntryLoc = MapSettings::ENTRY_FAR_FROM_ROOM;
	}
	else if(gameLevel->locationIndex == questMgr->questSecret->where
		&& questMgr->questSecret->state == Quest_Secret::SECRET_DROPPED_STONE && !inside->HaveNextEntry())
	{
		// secret
		Room* room = Room::Get();
		room->target = RoomTarget::PortalCreate;
		room->type = nullptr;
		room->size = Int2(7, 7);
		room->pos.x = room->pos.y = (settings.mapW - 7) / 2;
		room->connected.clear();
		room->index = 0;
		inside->specialRoom = 0;
		lvl.rooms.push_back(room);
		settings.prevEntryLoc = MapSettings::ENTRY_FAR_FROM_ROOM;
	}
	else if(gameLevel->location == questMgr->questEvil->targetLoc && questMgr->questEvil->evilState == Quest_Evil::State::GeneratedCleric)
	{
		// schody w krypcie 0 jak najdalej od �rodka
		settings.prevEntryLoc = MapSettings::ENTRY_FAR_FROM_ROOM;
	}

	if(questMgr->questOrcs2->orcsState == Quest_Orcs2::State::Accepted && gameLevel->location == questMgr->questOrcs->targetLoc
		&& dungeonLevel == gameLevel->location->GetLastLevel())
	{
		// search for room for cell
		settings.stop = true;
		bool first = true;
		int tries = 0;
		vector<Room*> possibleRooms;

		while(true)
		{
			mapGenerator.Generate(settings, !first);
			first = false;

			for(Room* room : lvl.rooms)
			{
				if(room->target == RoomTarget::None && room->connected.size() == 1)
					possibleRooms.push_back(room);
			}

			if(possibleRooms.empty())
			{
				++tries;
				if(tries == 100)
					throw "Failed to generate special room in dungeon!";
				else
					continue;
			}

			Room* room = RandomItem(possibleRooms);
			room->target = RoomTarget::Prison;
			Int2 pt = mapGenerator.GetConnectingTile(room, room->connected.front());
			Tile& p = mapGenerator.GetMap()[pt.x + pt.y * settings.mapW];
			p.type = DOORS;
			p.flags |= Tile::F_SPECIAL;

			mapGenerator.ContinueGenerating();
			break;
		}
	}
	else
		mapGenerator.Generate(settings);

	lvl.w = lvl.h = settings.mapW;
	lvl.map = mapGenerator.GetMap();
	lvl.prevEntryType = settings.prevEntryType;
	lvl.prevEntryPt = settings.prevEntryPt;
	lvl.prevEntryDir = settings.prevEntryDir;
	lvl.nextEntryType = settings.nextEntryType;
	lvl.nextEntryPt = settings.nextEntryPt;
	lvl.nextEntryDir = settings.nextEntryDir;

	// different texture in crypt treasure room
	if(!skipNormalHandling && Any(inside->target, HERO_CRYPT, MONSTER_CRYPT) && !inside->HaveNextEntry())
	{
		Room& r = *lvl.rooms[0];
		for(int y = 0; y < r.size.y; ++y)
		{
			for(int x = 0; x < r.size.x; ++x)
				lvl.map[r.pos.x + x + (r.pos.y + y) * lvl.w].flags |= Tile::F_SECOND_TEXTURE;
		}
	}

	if(eventHandler)
	{
		ScriptEvent e(EVENT_GENERATE);
		e.onGenerate.location = inside;
		e.onGenerate.mapSettings = nullptr;
		e.onGenerate.stage = 1;
		e.onGenerate.cancel = false;
		eventHandler->FireEvent(e);
	}

	if(inside->fromPortal)
		CreatePortal(lvl);
}

//=================================================================================================
void DungeonGenerator::CreatePortal(InsideLocationLevel& lvl)
{
	vector<pair<Int2, GameDirection>> goodPts;

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
						goodPts.push_back(std::make_pair(r.pos + Int2(x, y), dir));
#undef P
#undef B
				}
			}
		}

		if(goodPts.empty())
			continue;

		pair<Int2, GameDirection>& pt = goodPts[Rand() % goodPts.size()];

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
	if(gameLevel->location->group->IsEmpty())
		return;

	UnitGroup* group = GetGroup();
	int baseLevel;
	if(gameLevel->location->group->IsChallange())
		baseLevel = gameLevel->location->st; // all levels have max st
	else
		baseLevel = gameLevel->GetDifficultyLevel();

	Pooled<TmpUnitGroup> tmp;
	tmp->Fill(group, baseLevel);

	const bool canSpawnSlime = (baseLevel >= 3 && baseLevel <= 5);

	// chance for spawning units
	const int chanceForNone = 10,
		chanceFor1 = 20,
		chanceFor2 = 30,
		chanceFor3 = 40,
		chanceInCorridor = 25;

	assert(InRange(chanceForNone, 0, 100) && InRange(chanceFor1, 0, 100) && InRange(chanceFor2, 0, 100) && InRange(chanceFor3, 0, 100)
		&& InRange(chanceInCorridor, 0, 100) && chanceForNone + chanceFor1 + chanceFor2 + chanceFor3 == 100);

	const int chance[3] = { chanceForNone, chanceForNone + chanceFor1, chanceForNone + chanceFor1 + chanceFor2 };

	// spawn units
	InsideLocation* inside = (InsideLocation*)gameLevel->location;
	InsideLocationLevel& lvl = inside->GetLevelData();
	Int2 excludedPt = lvl.nextEntryPt;
	if(!inside->HaveNextEntry())
		excludedPt = Int2(-1000, -1000);

	for(RoomGroup& group : lvl.groups)
	{
		if(Any(group.target, RoomTarget::Treasury, RoomTarget::Prison))
			continue;

		int count;
		if(group.target == RoomTarget::Corridor)
		{
			assert(group.rooms.size() == 1u);
			if(Rand() % 100 < chanceInCorridor)
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

		Int2 awayPt;
		if(group.target == RoomTarget::EntryPrev)
			awayPt = lvl.prevEntryPt;
		else if(inside->fromPortal && group.target == RoomTarget::Portal)
			awayPt = PosToPt(inside->portal->pos);
		else
			awayPt = Int2(-1000, -1000);

		if(canSpawnSlime && Rand() % 20 == 0)
		{
			static UnitData* slime = UnitData::Get("slime");
			for(int i = 0; i < count; ++i)
			{
				Room& room = *lvl.rooms[RandomItem(group.rooms)];
				gameLevel->SpawnUnitInsideRoom(room, *slime, -1, awayPt, excludedPt);
			}
		}
		else
		{
			for(TmpUnitGroup::Spawn& spawn : tmp->Roll(baseLevel, count))
			{
				Room& room = *lvl.rooms[RandomItem(group.rooms)];
				gameLevel->SpawnUnitInsideRoom(room, *spawn.first, spawn.second, awayPt, excludedPt);
			}
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
	BaseLocation& base = gBaseLocations[inside->target];

	if(IsSet(base.options, BLO_LESS_FOOD))
		--mod;

	UnitGroup* group = GetGroup();
	mod += group->foodMod;

	if(mod <= 0)
		return;

	// get food list and base objects
	const ItemList& lis = ItemList::Get(group->orcFood ? "orcFood" : "normalFood");
	BaseObject* table = BaseObject::Get("table"),
		*shelves = BaseObject::Get("shelves");
	const Item* plate = Item::Get("plate");
	const Item* cup = Item::Get("cup");
	bool spawnGoldenCup = Rand() % 100 == 0;

	// spawn food
	LocationPart& locPart = *gameLevel->localPart;
	for(vector<Object*>::iterator it = locPart.objects.begin(), end = locPart.objects.end(); it != end; ++it)
	{
		Object& obj = **it;
		if(obj.base == table)
		{
			gameLevel->PickableItemBegin(locPart, obj);
			if(spawnGoldenCup)
			{
				spawnGoldenCup = false;
				gameLevel->PickableItemAdd(Item::Get("goldenCup"));
			}
			else
			{
				int count = Random(mod / 2, mod);
				if(count)
				{
					for(int i = 0; i < count; ++i)
						gameLevel->PickableItemAdd(lis.Get());
				}
			}
			if(Rand() % 3 == 0)
				gameLevel->PickableItemAdd(plate);
			if(Rand() % 3 == 0)
				gameLevel->PickableItemAdd(cup);
		}
		else if(obj.base == shelves)
		{
			int count = Random(mod, mod * 3 / 2);
			if(count)
			{
				gameLevel->PickableItemBegin(locPart, obj);
				for(int i = 0; i < count; ++i)
					gameLevel->PickableItemAdd(lis.Get());
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
		assert(group->isList);
		group = group->entries[dungeonLevel % group->entries.size()].group;
	}
	return group;
}
