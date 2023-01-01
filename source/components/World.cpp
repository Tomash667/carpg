#include "Pch.h"
#include "World.h"

#include "BitStreamFunc.h"
#include "Camp.h"
#include "Cave.h"
#include "City.h"
#include "Encounter.h"
#include "Game.h"
#include "GameGui.h"
#include "Language.h"
#include "Level.h"
#include "LoadingHandler.h"
#include "LocationHelper.h"
#include "MultiInsideLocation.h"
#include "Net.h"
#include "News.h"
#include "OffscreenLocation.h"
#include "Quest.h"
#include "QuestManager.h"
#include "Quest_Scripted.h"
#include "ScriptManager.h"
#include "Team.h"
#include "WorldMapGui.h"

#include <scriptarray/scriptarray.h>

//-----------------------------------------------------------------------------
const float World::TRAVEL_SPEED = 28.f;
const float World::MAP_KM_RATIO = 1.f / 3; // 1200 pixels = 400 km
const float WORLD_BORDER = 50.f;
const int DEF_WORLD_SIZE = 1200;
World* world;
vector<string> txLocationStart, txLocationEnd;

//-----------------------------------------------------------------------------
// used temporary before deciding how many levels location should have (and use Single or MultiInsideLocation)
struct TmpLocation : public Location
{
	TmpLocation() : Location(false) {}

	void Apply(vector<std::reference_wrapper<LocationPart>>& parts) override {}
	void Write(BitStreamWriter& f) override {}
	bool Read(BitStreamReader& f) override { return false; }
};

//=================================================================================================
void World::LoadLanguage()
{
	// general
	Language::Section s = Language::GetSection("WorldMap");
	txDate = s.Get("dateFormat");
	s.GetArray(txMonth, "month");

	// encounters
	s = Language::GetSection("Encounters");
	txEncCrazyMage = s.Get("encCrazyMage");
	txEncCrazyHeroes = s.Get("encCrazyHeroes");
	txEncCrazyCook = s.Get("encCrazyCook");
	txEncMerchant = s.Get("encMerchant");
	txEncHeroes = s.Get("encHeroes");
	txEncSingleHero = s.Get("encSingleHero");
	txEncBanditsAttackTravelers = s.Get("encBanditsAttackTravelers");
	txEncHeroesAttack = s.Get("encHeroesAttack");
	txEncEnemiesCombat = s.Get("encEnemiesCombat");

	// location names
	s = Language::GetSection("Locations");
	txCamp = s.Get("camp");
	txCave = s.Get("cave");
	txCity = s.Get("city");
	txCrypt = s.Get("crypt");
	txDungeon = s.Get("dungeon");
	txForest = s.Get("forest");
	txVillage = s.Get("village");
	txMoonwell = s.Get("moonwell");
	txOtherness = s.Get("otherness");
	txRandomEncounter = s.Get("randomEncounter");
	txTower = s.Get("tower");
	txLabyrinth = s.Get("labyrinth");
	txAcademy = s.Get("academy");
	txHuntersCamp = s.Get("huntersCamp");
	txHills = s.Get("hills");
}

//=================================================================================================
void World::Init()
{
	gui = gameGui->worldMap;
}

//=================================================================================================
void World::Cleanup()
{
	Reset();
}

//=================================================================================================
void World::OnNewGame()
{
	travelLocation = nullptr;
	emptyLocations = 0;
	createCamp = Random(5, 10);
	encounterChance = 0.f;
	travelDir = Random(MAX_ANGLE);
	date = Date(100, 0, 0);
	startDate = date;
	worldtime = 0;
	dayTimer = 0.f;
	revealTimer = 0.f;
}

//=================================================================================================
void World::Reset()
{
	DeleteElements(locations);
	DeleteElements(encounters);
	DeleteElements(globalEncounters);
	DeleteElements(news);
}

//=================================================================================================
void World::Update(int days, UpdateMode mode)
{
	assert(Net::IsLocal());
	if(mode == UM_TRAVEL)
		assert(days == 1);
	else
		assert(days > 0);

	UpdateDate(days);
	SpawnCamps(days);
	UpdateEncounters();
	if(Any(state, State::INSIDE_LOCATION, State::INSIDE_ENCOUNTER))
		gameLevel->Update();
	UpdateLocations();
	UpdateNews();
	questMgr->Update(days);
	team->Update(days, mode);

	// end of game
	if(date.year >= startDate.year + 60)
	{
		Info("Game over: you are too old.");
		gameGui->CloseAllPanels();
		game->endOfGame = true;
		game->deathFade = 0.f;
		if(Net::IsOnline())
		{
			Net::PushChange(NetChange::GAME_STATS);
			Net::PushChange(NetChange::END_OF_GAME);
		}
	}
}

//=================================================================================================
void World::UpdateDate(int days)
{
	worldtime += days;
	date.AddDays(days);
	if(Net::IsOnline())
		Net::PushChange(NetChange::WORLD_TIME);
}

//=================================================================================================
void World::SpawnCamps(int days)
{
	createCamp -= days;
	if(createCamp <= 0)
	{
		createCamp = Random(5, 10);
		int index = Rand() % locations.size(),
			start = index;
		while(true)
		{
			Location* loc = locations[index];
			if(loc && loc->state != LS_CLEARED && loc->type == L_DUNGEON && loc->group->haveCamps)
			{
				const Vec2 pos = FindPlace(loc->pos, 128.f);
				CreateCamp(pos, loc->group);
				break;
			}

			index = (index + 1) % locations.size();
			if(index == start)
			{
				// no active dungeons, highly unlikely
				break;
			}
		}
	}
}

//=================================================================================================
void World::UpdateEncounters()
{
	for(vector<Encounter*>::iterator it = encounters.begin(), end = encounters.end(); it != end; ++it)
	{
		Encounter* e = *it;
		if(e && e->timed && e->quest->IsTimedout())
		{
			e->quest->OnTimeout(TIMEOUT_ENCOUNTER);
			static_cast<Quest_Encounter*>(e->quest)->enc = -1;
			delete *it;
			if(it + 1 == end)
			{
				encounters.pop_back();
				break;
			}
			else
				*it = nullptr;
		}
	}
}

//=================================================================================================
void World::UpdateLocations()
{
	for(auto it = locations.begin(), end = locations.end(); it != end; ++it)
	{
		Location* loc = *it;
		if(!loc || loc->activeQuest || loc->type == L_ENCOUNTER)
			continue;
		if(loc->type == L_CAMP)
		{
			Camp* camp = static_cast<Camp*>(loc);
			if(worldtime - camp->createTime >= 30
				&& currentLocation != loc // don't remove when team is inside
				&& travelLocation != loc) // don't remove when traveling to
			{
				// remove camp
				DeleteCamp(camp, false);
				if(it + 1 == end)
				{
					locations.pop_back();
					break;
				}
				else
				{
					++emptyLocations;
					*it = nullptr;
				}
			}
		}
		else if(loc->lastVisit != -1 && worldtime - loc->lastVisit >= 30)
		{
			loc->reset = true;
			if(loc->state == LS_CLEARED)
				loc->state = LS_VISITED;
			if(loc->type == L_DUNGEON)
			{
				InsideLocation* inside = static_cast<InsideLocation*>(loc);
				inside->group = gBaseLocations[inside->target].GetRandomGroup();
				if(inside->IsMultilevel())
					static_cast<MultiInsideLocation*>(inside)->Reset();
			}
		}
	}
}

//=================================================================================================
void World::UpdateNews()
{
	DeleteElements(news, [this](News* news) { return worldtime - news->addTime > 30; });
}

//=================================================================================================
Location* World::CreateCamp(const Vec2& pos, UnitGroup* group)
{
	assert(group);

	Camp* loc = new Camp;
	loc->state = LS_UNKNOWN;
	loc->activeQuest = nullptr;
	loc->type = L_CAMP;
	loc->lastVisit = -1;
	loc->reset = false;
	loc->group = group;
	loc->pos = pos;
	loc->createTime = worldtime;
	SetLocationImageAndName(loc);

	loc->st = GetTileSt(loc->pos);
	if(loc->st > 3)
		loc->st = 3;

	AddLocation(loc);

	return loc;
}

//=================================================================================================
Location* World::CreateLocation(LOCATION type, int levels, int cityTarget)
{
	switch(type)
	{
	case L_CITY:
		{
			City* city = new City;
			city->target = cityTarget;
			return city;
		}
	case L_CAVE:
		return new Cave;
	case L_CAMP:
		return new Camp;
	case L_OUTSIDE:
	case L_ENCOUNTER:
		return new OutsideLocation;
	case L_DUNGEON:
		{
			Location* loc;
			if(levels == -1)
				loc = new TmpLocation;
			else
			{
				assert(levels != 0);
				if(levels == 1)
					loc = new SingleInsideLocation;
				else
					loc = new MultiInsideLocation(levels);
			}
			return loc;
		}
	default:
		assert(0);
		return nullptr;
	}
}

//=================================================================================================
// Create location
// If range<0 then position is random and range is positive
// Dungeon levels
//	-1 - random count
//	0 - minimum randomly generated
//	9 - maximum randomly generated
//	other - used number
Location* World::CreateLocation(LOCATION type, const Vec2& pos, int target, int dungeonLevels)
{
	int levels = -1;
	if(type == L_DUNGEON)
	{
		assert(target != -1);
		BaseLocation& base = gBaseLocations[target];
		if(dungeonLevels == -1)
			levels = base.levels.Random();
		else if(dungeonLevels == 0)
			levels = base.levels.x;
		else if(dungeonLevels == 9)
			levels = base.levels.y;
		else
			levels = dungeonLevels;
	}
	else if(target == -1)
	{
		if(type == L_CAVE)
			target = CAVE;
		else
			target = 0;
	}

	Location* loc = CreateLocation(type, levels);
	loc->pos = pos;
	loc->type = type;
	loc->target = target;

	if(type == L_DUNGEON)
	{
		InsideLocation* inside = static_cast<InsideLocation*>(loc);
		inside->group = gBaseLocations[target].GetRandomGroup();
		if(target == LABYRINTH)
		{
			inside->st = GetTileSt(loc->pos);
			if(inside->st < 6)
				inside->st = 6;
			else if(inside->st > 14)
				inside->st = 14;
		}
		else
		{
			inside->st = GetTileSt(loc->pos);
			if(inside->st < 3)
				inside->st = 3;
		}
	}
	else
	{
		loc->st = GetTileSt(loc->pos);
		if(loc->st < 2)
			loc->st = 2;
		switch(type)
		{
		case L_CITY:
			{
				City* city = static_cast<City*>(loc);
				city->group = UnitGroup::empty;
				city->st = 1;

				LocalVector<Building*> buildings;
				city->GenerateCityBuildings(buildings.Get(), true);
				city->buildings.reserve(buildings.size());
				for(Building* b : buildings)
				{
					CityBuilding& cb = Add1(city->buildings);
					cb.building = b;
				}
			}
			break;
		case L_OUTSIDE:
			loc->group = UnitGroup::Get("forest");
			break;
		case L_CAVE:
			loc->group = UnitGroup::Get("cave");
			break;
		default:
			assert(0);
			loc->group = UnitGroup::empty;
			break;
		}
	}

	SetLocationImageAndName(loc);
	AddLocation(loc);
	return loc;
}

//=================================================================================================
void World::AddLocations(uint count, AddLocationsCallback clbk, float validDist)
{
	for(uint i = 0; i < count; ++i)
	{
		for(uint j = 0; j < 100; ++j)
		{
			Vec2 pt = Vec2::Random(WORLD_BORDER, float(worldSize) - WORLD_BORDER);
			bool ok = true;

			// disallow when near other location
			for(Location* l : locations)
			{
				float dist = Vec2::Distance(pt, l->pos);
				if(dist < validDist)
				{
					ok = false;
					break;
				}
			}
			if(!ok)
				continue;

			LOCATION type = clbk(i);
			Location* l = CreateLocation(type, -1);
			l->type = type;
			l->pos = pt;
			l->state = LS_UNKNOWN;
			l->index = locations.size();
			locations.push_back(l);
			break;
		}
	}
}

//=================================================================================================
int World::AddLocation(Location* loc)
{
	assert(loc);

	if(emptyLocations)
	{
		--emptyLocations;
		int index = locations.size() - 1;
		for(vector<Location*>::reverse_iterator rit = locations.rbegin(), rend = locations.rend(); rit != rend; ++rit, --index)
		{
			if(!*rit)
			{
				*rit = loc;
				loc->index = index;
				if(Net::IsOnline() && !net->prepareWorld)
				{
					NetChange& c = Add1(Net::changes);
					c.type = NetChange::ADD_LOCATION;
					c.id = index;
				}
				return index;
			}
		}
		assert(0);
		return -1;
	}
	else
	{
		if(Net::IsOnline() && !net->prepareWorld)
		{
			NetChange& c = Add1(Net::changes);
			c.type = NetChange::ADD_LOCATION;
			c.id = locations.size();
		}
		loc->index = locations.size();
		locations.push_back(loc);
		return loc->index;
	}
}

//=================================================================================================
void World::AddLocationAtIndex(Location* loc)
{
	assert(loc && loc->index >= 0);
	if(loc->index >= (int)locations.size())
		locations.resize(loc->index + 1, nullptr);
	assert(!locations[loc->index]);
	locations[loc->index] = loc;
}

//=================================================================================================
void World::RemoveLocation(int index)
{
	if(Net::IsClient())
	{
		assert(VerifyLocation(index));
		delete locations[index];
	}
	if(index == locations.size() - 1)
		locations.pop_back();
	else
	{
		locations[index] = nullptr;
		++emptyLocations;
	}
}

//=================================================================================================
void World::GenerateWorld()
{
	auto isDistanceOk = [&](const Vec2 &pos, const float minDistance) {
		for(Location* loc : locations)
		{
			const float dist = Vec2::DistanceSquared(loc->pos, pos);
			if(dist < minDistance * minDistance)
				return false;
		}
		return true;
	};
	startup = true;
	worldSize = DEF_WORLD_SIZE;
	worldBounds = Vec2(WORLD_BORDER, worldSize - WORLD_BORDER);

	// create capital
	const Vec2 pos = Vec2::Random(float(worldSize) * 0.4f, float(worldSize) * 0.6f);
	CreateCity(pos, CAPITAL);

	// create more cities
	for(uint i = 0, count = Random(3u, 4u); i < count; ++i)
	{
		for(int tries = 0; tries < 20; ++tries)
		{
			const Vec2 parentPos = locations[Rand() % locations.size()]->pos;
			const float rot = Random(MAX_ANGLE);
			const float dist = Random(300.f, 400.f);
			const Vec2 pos = parentPos + Vec2(cos(rot) * dist, sin(rot) * dist);

			if(pos.x < worldBounds.x || pos.y < worldBounds.x || pos.x > worldBounds.y || pos.y > worldBounds.y)
				continue;
			if(isDistanceOk(pos, 250.f))
			{
				CreateCity(pos, CITY);
				break;
			}
		}
	}

	// player starts in one of cities
	const uint cities = locations.size();
	const uint startLocation = Rand() % cities;

	// create villages
	for(uint i = 0, count = Random(10u, 15u); i < count; ++i)
	{
		for(int tries = 0; tries < 20; ++tries)
		{
			const Vec2 parentPos = locations[Rand() % cities]->pos;
			const float rot = Random(MAX_ANGLE);
			const float dist = Random(100.f, 250.f);
			const Vec2 pos = parentPos + Vec2(cos(rot) * dist, sin(rot) * dist);

			if(pos.x < worldBounds.x || pos.y < worldBounds.x || pos.x > worldBounds.y || pos.y > worldBounds.y)
				continue;

			if(isDistanceOk(pos, 100.0f))
			{
				CreateCity(pos, VILLAGE);
				break;
			}
		}
	}

	// create desolated villages
	for(uint i = 0, count = 3; i < count; ++i)
	{
		for(int tries = 0; tries < 50; ++tries)
		{
			const Vec2 pos = Vec2::Random(worldBounds.x, worldBounds.y);

			if(isDistanceOk(pos, 400.0f))
			{
				CreateCity(pos, VILLAGE);
				break;
			}
		}
	}
	settlements = locations.size();

	// generate guaranteed locations, one of each type & target
	static const LOCATION guaranteed[] = {
		L_OUTSIDE, L_OUTSIDE, L_OUTSIDE, L_OUTSIDE,
		L_CAVE,
		L_DUNGEON, L_DUNGEON, L_DUNGEON, L_DUNGEON, L_DUNGEON, L_DUNGEON, L_DUNGEON, L_DUNGEON, L_DUNGEON, L_DUNGEON
	};
	AddLocations(countof(guaranteed), [](uint index) { return guaranteed[index]; }, 40.f);

	// generate other locations
	static const LOCATION locs[] = {
		L_CAVE,
		L_DUNGEON,
		L_DUNGEON,
		L_DUNGEON,
		L_DUNGEON,
		L_OUTSIDE
	};
	AddLocations(120 - locations.size(), [](uint index) { return locs[Rand() % countof(locs)]; }, 40.f);

	// create location for random encounter
	encounterLoc = new OutsideLocation;
	encounterLoc->index = locations.size();
	encounterLoc->pos = Vec2(-1000, -1000);
	encounterLoc->state = LS_HIDDEN;
	encounterLoc->type = L_ENCOUNTER;
	SetLocationImageAndName(encounterLoc);
	locations.push_back(encounterLoc);

	// create offscreen location
	offscreenLoc = new OffscreenLocation;
	offscreenLoc->index = locations.size();
	offscreenLoc->pos = Vec2(-1000, -1000);
	offscreenLoc->state = LS_HIDDEN;
	offscreenLoc->type = L_OFFSCREEN;
	offscreenLoc->group = UnitGroup::empty;
	locations.push_back(offscreenLoc);

	CalculateTiles();

	// generate locations content
	int index = 0, guaranteedDungeon = 0, guaranteedOutside = 0;
	UnitGroup* forestGroup = UnitGroup::Get("forest");
	UnitGroup* caveGroup = UnitGroup::Get("cave");
	const bool knowsHuntersCamp = team->HaveClass(Class::TryGet("hunter"));
	for(vector<Location*>::iterator it = locations.begin(), end = locations.end(); it != end; ++it, ++index)
	{
		if(!*it)
			continue;

		Location& loc = **it;
		switch(loc.type)
		{
		case L_CITY:
			{
				City& city = (City&)loc;
				city.citizens = 0;
				city.citizensWorld = 0;
				city.st = 1;
				city.group = UnitGroup::empty;
				LocalVector<Building*> buildings;
				city.GenerateCityBuildings(buildings.Get(), true);
				city.buildings.reserve(buildings.size());
				for(Building* b : buildings)
				{
					CityBuilding& cb = Add1(city.buildings);
					cb.building = b;
				}
			}
			break;
		case L_DUNGEON:
			{
				InsideLocation* inside;

				int target;
				switch(guaranteedDungeon++)
				{
				case 0:
					target = HUMAN_FORT;
					break;
				case 1:
					target = DWARF_FORT;
					break;
				case 2:
					target = MAGE_TOWER;
					break;
				case 3:
					target = BANDITS_HIDEOUT;
					break;
				case 4:
					target = OLD_TEMPLE;
					break;
				case 5:
					target = VAULT;
					break;
				case 6:
					target = NECROMANCER_BASE;
					break;
				case 7:
					target = LABYRINTH;
					break;
				case 8:
					target = HERO_CRYPT;
					break;
				case 9:
					target = MONSTER_CRYPT;
					break;
				default:
					if(Rand() % 4 == 0)
						target = Rand() % 2 == 0 ? HERO_CRYPT : MONSTER_CRYPT;
					else
					{
						switch(Rand() % 14)
						{
						default:
						case 0:
						case 1:
							target = HUMAN_FORT;
							break;
						case 2:
						case 3:
							target = DWARF_FORT;
							break;
						case 4:
						case 5:
							target = MAGE_TOWER;
							break;
						case 6:
						case 7:
							target = BANDITS_HIDEOUT;
							break;
						case 8:
						case 9:
							target = OLD_TEMPLE;
							break;
						case 10:
							target = VAULT;
							break;
						case 11:
						case 12:
							target = NECROMANCER_BASE;
							break;
						case 13:
							target = LABYRINTH;
							break;
						}
					}
					break;
				}

				BaseLocation& base = gBaseLocations[target];
				int levels = base.levels.Random();

				if(levels == 1)
					inside = new SingleInsideLocation;
				else
					inside = new MultiInsideLocation(levels);

				inside->type = loc.type;
				inside->state = loc.state;
				inside->pos = loc.pos;
				inside->index = loc.index;
				delete &loc;
				*it = inside;

				inside->target = target;
				inside->group = base.GetRandomGroup();
				int& st = GetTileSt(inside->pos);
				if(target == LABYRINTH)
				{
					if(st < 6)
						st = 6;
					else if(st > 14)
						st = 14;
				}
				else
				{
					if(st < 3)
						st = 3;
				}
				inside->st = st;
			}
			break;
		case L_ENCOUNTER:
			loc.st = 10;
			loc.group = UnitGroup::empty;
			break;
		case L_OUTSIDE:
			switch(guaranteedOutside++)
			{
			case 0:
				loc.target = MOONWELL;
				break;
			case 1:
				loc.target = HUNTERS_CAMP;
				break;
			case 2:
				loc.target = FOREST;
				break;
			case 3:
				loc.target = HILLS;
				break;
			default:
				loc.target = (Rand() % 2 == 0 ? FOREST : HILLS);
				break;
			}

			switch(loc.target)
			{
			case FOREST:
			case HILLS:
				{
					int& st = GetTileSt(loc.pos);
					if(st < 2)
						st = 2;
					else if(st > 10)
						st = 10;
					loc.st = st;
					loc.group = forestGroup;
				}
				break;
			case MOONWELL:
				loc.target = MOONWELL;
				loc.st = 10;
				loc.group = forestGroup;
				GetTileSt(loc.pos) = 10;
				break;
			case HUNTERS_CAMP:
				loc.target = HUNTERS_CAMP;
				loc.st = 3;
				loc.activeQuest = ACTIVE_QUEST_HOLDER;
				loc.group = UnitGroup::empty;
				if(knowsHuntersCamp)
					loc.state = LS_KNOWN;
				break;
			}
			break;
		case L_CAVE:
			{
				int& st = GetTileSt(loc.pos);
				if(st < 2)
					st = 2;
				else if(st > 10)
					st = 10;
				loc.st = st;
				loc.group = caveGroup;
			}
			break;
		}

		SetLocationImageAndName(*it);
	}

	SmoothTiles();

	this->startLocation = locations[startLocation];
	tomirSpawned = false;
}

//=================================================================================================
void World::StartInLocation()
{
	state = State::ON_MAP;
	currentLocationIndex = startLocation->index;
	currentLocation = startLocation;
	currentLocation->state = LS_VISITED;
	worldPos = currentLocation->pos;
	gameLevel->locationIndex = currentLocationIndex;
	gameLevel->location = currentLocation;
	startup = false;

	// reveal near locations
	const Vec2& startPos = startLocation->pos;
	for(vector<Location*>::iterator it = locations.begin(), end = locations.end(); it != end; ++it)
	{
		if(!*it)
			continue;

		Location& loc = **it;
		if(loc.state == LS_UNKNOWN && Vec2::Distance(startPos, loc.pos) <= 150.f)
			loc.state = LS_KNOWN;
	}
}

//=================================================================================================
void World::CalculateTiles()
{
	const int ts = worldSize / TILE_SIZE;
	tiles.resize(ts * ts);

	// set from near locations
	for(int y = 0; y < ts; ++y)
	{
		for(int x = 0; x < ts; ++x)
		{
			const int index = x + y * ts;
			int st = Random(5, 15);
			const Vec2 pos(float(x * TILE_SIZE) + 0.5f * TILE_SIZE, float(y * TILE_SIZE) + 0.5f * TILE_SIZE);
			for(uint i = 0; i < locations.size(); ++i)
			{
				Location* loc = locations[i];
				if(!Any(loc->type, L_CITY, L_DUNGEON))
					continue;
				const float dist = Vec2::Distance(pos, loc->pos);
				if(loc->type == L_CITY)
				{
					if(static_cast<City*>(loc)->IsVillage())
					{
						if(dist < 100)
							st -= 10 - int(dist / 10);
					}
					else
					{
						if(dist < 200)
							st -= 15 - int(dist / 20);
						else if(dist < 400)
							st -= 5 - int((dist - 200) / 40);
					}
				}
				else
				{
					if(dist < 100)
						st += int(dist / 20);
				}
			}
			tiles[index] = st;
		}
	}

	SmoothTiles();
}

//=================================================================================================
void World::SmoothTiles()
{
	int ts = worldSize / TILE_SIZE;

	// smooth
	for(int y = 0; y < ts; ++y)
	{
		for(int x = 0; x < ts; ++x)
		{
			int count = 1;
			int st = tiles[x + y * ts];
			if(x > 0)
			{
				++count;
				st += tiles[x - 1 + y * ts];
				if(y > 0)
				{
					++count;
					st += tiles[x - 1 + (y - 1) * ts];
				}
				if(y < ts - 1)
				{
					++count;
					st += tiles[x - 1 + (y + 1) * ts];
				}
			}
			if(x < ts - 1)
			{
				++count;
				st += tiles[x + 1 + y * ts];
				if(y > 0)
				{
					++count;
					st += tiles[x + 1 + (y - 1) * ts];
				}
				if(y < ts - 1)
				{
					++count;
					st += tiles[x + 1 + (y + 1) * ts];
				}
			}
			if(y > 0)
			{
				++count;
				st += tiles[x + (y - 1) * ts];
			}
			if(y < ts - 1)
			{
				++count;
				st += tiles[x + (y + 1) * ts];
			}
			tiles[x + y * ts] = st / count;
		}
	}

	// clamp values
	for(int i = 0; i < ts * ts; ++i)
		tiles[i] = Clamp(tiles[i], 2, 16);
}

//=================================================================================================
void World::CreateCity(const Vec2& pos, int target)
{
	Location* l = CreateLocation(L_CITY, -1, target);
	l->type = L_CITY;
	l->pos = pos;
	l->state = LS_KNOWN;
	l->index = locations.size();
	locations.push_back(l);
}

//=================================================================================================
void World::SetLocationImageAndName(Location* l)
{
	// set image & prefix
	switch(l->type)
	{
	case L_CITY:
		switch(l->target)
		{
		case VILLAGE:
		case VILLAGE_EMPTY:
		case VILLAGE_DESTROYED:
		case VILLAGE_DESTROYED2:
			l->image = LI_VILLAGE;
			l->name = txVillage;
			break;
		default:
		case CITY:
			l->image = LI_CITY;
			l->name = txCity;
			break;
		case CAPITAL:
			l->image = LI_CAPITAL;
			l->name = txCity;
			break;
		}
		break;
	case L_CAVE:
		l->image = LI_CAVE;
		l->name = txCave;
		break;
	case L_CAMP:
		l->image = LI_CAMP;
		l->name = txCamp;
		return;
	case L_OUTSIDE:
		switch(l->target)
		{
		default:
		case FOREST:
			l->image = LI_FOREST;
			l->name = txForest;
			break;
		case HILLS:
			l->image = LI_HILLS;
			l->name = txHills;
			break;
		case MOONWELL:
			l->image = LI_MOONWELL;
			l->name = txMoonwell;
			return;
		case ACADEMY:
			l->image = LI_ACADEMY;
			l->name = txAcademy;
			return;
		case HUNTERS_CAMP:
			l->image = LI_HUNTERS_CAMP;
			l->name = txHuntersCamp;
			return;
		}
		break;
	case L_ENCOUNTER:
		l->image = LI_FOREST;
		l->name = txRandomEncounter;
		return;
	case L_DUNGEON:
		switch(l->target)
		{
		case HERO_CRYPT:
		case MONSTER_CRYPT:
			l->image = LI_CRYPT;
			l->name = txCrypt;
			break;
		case LABYRINTH:
			l->image = LI_LABYRINTH;
			l->name = txLabyrinth;
			break;
		case MAGE_TOWER:
			l->image = LI_TOWER;
			l->name = txTower;
			break;
		default:
			l->image = (Rand() % 2 == 0 ? LI_DUNGEON : LI_DUNGEON2);
			l->name = txDungeon;
			break;
		}
		break;
	case L_OFFSCREEN:
		l->image = LI_FOREST;
		l->name = "Offscreen";
		break;
	default:
		assert(0);
		l->image = LI_FOREST;
		l->name = txOtherness;
		return;
	}

	// set name
	l->name += ' ';
	uint len = l->name.size();
	while(true)
	{
		cstring s1 = RandomItem(txLocationStart).c_str();
		cstring s2;
		do
		{
			s2 = RandomItem(txLocationEnd).c_str();
		}
		while(_stricmp(s1, s2) == 0);
		l->name += s1;
		if(l->name[l->name.length() - 1] == s2[0])
			l->name += (s2 + 1);
		else
			l->name += s2;

		bool exists = false;
		for(Location* l2 : locations)
		{
			if(l2 && l != l2 && l2->name == l->name)
			{
				exists = true;
				break;
			}
		}
		if(!exists)
			break;
		l->name.resize(len);
	}
}

//=================================================================================================
void World::Save(GameWriter& f)
{
	f << state;
	f << date;
	f << startDate;
	f << worldtime;
	f << dayTimer;
	f << worldSize;
	f << currentLocationIndex;

	f << locations.size();
	Location* current;
	if(state == State::INSIDE_LOCATION || state == State::INSIDE_ENCOUNTER)
		current = currentLocation;
	else
		current = nullptr;
	byte checkId = 0;
	for(Location* loc : locations)
	{
		if(!loc)
			f << L_NULL;
		else
		{
			f << loc->type;
			if(loc->type == L_DUNGEON)
				f << loc->GetLastLevel() + 1;
			f.isLocal = (current == loc);
			loc->Save(f);
		}
		f << checkId;
		++checkId;
	}
	f.isLocal = false;

	f << emptyLocations;
	f << createCamp;
	f << worldPos;
	f << revealTimer;
	f << encounterChance;
	f << settlements;
	f << travelDir;
	if(state == State::INSIDE_ENCOUNTER)
	{
		f << travelLocation;
		f << travelStartPos;
		f << travelTargetPos;
		f << travelTimer;
	}
	f << encounters.size();
	for(Encounter* enc : encounters)
	{
		if(!enc || !enc->scripted)
			f << false;
		else
		{
			f << true;
			f << enc->pos;
			f << enc->chance;
			f << enc->range;
			f << enc->quest->id;
			if(enc->dialog)
				f << enc->dialog->id;
			else
				f.Write0();
			f << enc->group;
			if(enc->pooledString)
				f << *enc->pooledString;
			else
				f.Write0();
			f << enc->st;
			f << enc->dontAttack;
			f << enc->timed;
		}
	}
	f << tiles;

	f << news.size();
	for(News* n : news)
	{
		f << n->addTime;
		f.WriteString2(n->text);
	}

	f << tomirSpawned;
}

//=================================================================================================
void World::Load(GameReader& f, LoadingHandler& loading)
{
	f >> state;
	f >> date;
	if(LOAD_VERSION >= V_0_15)
		f >> startDate;
	else
		startDate = Date(100, 0, 0);
	f >> worldtime;
	f >> dayTimer;
	f >> worldSize;
	worldBounds = Vec2(WORLD_BORDER, worldSize - 16.f);
	f >> currentLocationIndex;
	LoadLocations(f, loading);
	LoadNews(f);
	if(LOAD_VERSION < V_0_12)
		f.Skip<bool>(); // old first_city
	if(LOAD_VERSION < V_0_17)
		f.SkipVector<Int2>(); // old boss_levels
	f >> tomirSpawned;
}

//=================================================================================================
void World::LoadLocations(GameReader& f, LoadingHandler& loading)
{
	byte readId,
		checkId = 0;

	uint count = f.Read<uint>();
	locations.resize(count);
	int index = -1;
	int currentIndex;
	if(state == State::INSIDE_LOCATION || state == State::INSIDE_ENCOUNTER)
		currentIndex = currentLocationIndex;
	else
		currentIndex = -1;
	int step = 0;
	for(Location*& loc : locations)
	{
		++index;

		if(LOAD_VERSION >= V_0_12)
		{
			LOCATION type;
			f >> type;

			if(type != L_NULL)
			{
				switch(type)
				{
				case L_OUTSIDE:
					loc = new OutsideLocation;
					break;
				case L_ENCOUNTER:
					encounterLoc = new OutsideLocation;
					loc = encounterLoc;
					break;
				case L_CITY:
					loc = new City;
					break;
				case L_CAVE:
					loc = new Cave;
					break;
				case L_DUNGEON:
					{
						int levels = f.Read<int>();
						if(levels == 1)
							loc = new SingleInsideLocation;
						else
							loc = new MultiInsideLocation(levels);
					}
					break;
				case L_CAMP:
					loc = new Camp;
					break;
				case L_OFFSCREEN:
					offscreenLoc = new OffscreenLocation;
					loc = offscreenLoc;
					break;
				default:
					assert(0);
					loc = new OutsideLocation;
					break;
				}

				f.isLocal = (currentIndex == index);
				loc->type = type;
				loc->index = index;
				loc->Load(f);
			}
			else
				loc = nullptr;
		}
		else
		{
			old::LOCATION_TOKEN locToken;
			f >> locToken;

			if(locToken != old::LT_NULL)
			{
				switch(locToken)
				{
				case old::LT_OUTSIDE:
					loc = new OutsideLocation;
					break;
				case old::LT_CITY:
				case old::LT_VILLAGE_OLD:
					loc = new City;
					break;
				case old::LT_CAVE:
					loc = new Cave;
					break;
				case old::LT_SINGLE_DUNGEON:
					loc = new SingleInsideLocation;
					break;
				case old::LT_MULTI_DUNGEON:
					{
						int levels = f.Read<int>();
						loc = new MultiInsideLocation(levels);
					}
					break;
				case old::LT_CAMP:
					loc = new Camp;
					break;
				default:
					assert(0);
					loc = new OutsideLocation;
					break;
				}

				f.isLocal = (currentIndex == index);
				loc->index = index;
				loc->Load(f);
			}
			else
				loc = nullptr;
		}

		if(step == 0)
		{
			if(index >= int(count) / 4)
			{
				++step;
				loading.Step();
			}
		}
		else if(step == 1)
		{
			if(index >= int(count) / 2)
			{
				++step;
				loading.Step();
			}
		}
		else if(step == 2)
		{
			if(index >= int(count) * 3 / 4)
			{
				++step;
				loading.Step();
			}
		}

		f >> readId;
		if(readId != checkId)
			throw Format("Error while reading location %s (%d).", loc ? loc->name.c_str() : "nullptr", index);
		++checkId;
	}
	f.isLocal = false;
	f >> emptyLocations;
	f >> createCamp;
	if(LOAD_VERSION < V_0_11 && createCamp > 10)
		createCamp = 10;
	f >> worldPos;
	f >> revealTimer;
	if(LOAD_VERSION < V_0_14_1 && revealTimer < 0)
		revealTimer = 0;
	f >> encounterChance;
	f >> settlements;
	if(LOAD_VERSION < V_0_14)
	{
		// old encounterLoc
		const int encounterLocIndex = f.Read<int>();
		encounterLoc = static_cast<OutsideLocation*>(locations[encounterLocIndex]);

		// create offscreen location
		offscreenLoc = new OffscreenLocation;
		offscreenLoc->index = locations.size();
		offscreenLoc->pos = Vec2(-1000, -1000);
		offscreenLoc->state = LS_HIDDEN;
		offscreenLoc->type = L_OFFSCREEN;
		offscreenLoc->group = UnitGroup::empty;
		locations.push_back(offscreenLoc);
	}
	f >> travelDir;
	if(state == State::INSIDE_ENCOUNTER)
	{
		f >> travelLocation;
		f >> travelStartPos;
		f >> travelTargetPos;
		f >> travelTimer;
	}

	// encounters
	encounters.resize(f.Read<uint>(), nullptr);
	index = 0;
	for(Encounter*& enc : encounters)
	{
		bool scripted;
		f >> scripted;
		if(scripted)
		{
			enc = new Encounter(nullptr);
			enc->index = index;
			f >> enc->pos;
			f >> enc->chance;
			f >> enc->range;
			int questId;
			f >> questId;
			const string& dialogId = f.ReadString1();
			if(!dialogId.empty())
			{
				string* str = StringPool.Get();
				*str = dialogId;
				questMgr->AddQuestRequest(questId, &enc->quest, [enc, str]
				{
					enc->dialog = static_cast<Quest_Scripted*>(enc->quest)->GetDialog(*str);
					StringPool.Free(str);
				});
			}
			else
				questMgr->AddQuestRequest(questId, &enc->quest);
			f >> enc->group;
			const string& text = f.ReadString1();
			if(text.empty())
			{
				enc->text = nullptr;
				enc->pooledString = nullptr;
			}
			else
			{
				enc->pooledString = StringPool.Get();
				*enc->pooledString = text;
				enc->text = enc->pooledString->c_str();
			}
			f >> enc->st;
			f >> enc->dontAttack;
			f >> enc->timed;
		}
		++index;
	}

	// fix camp spawned over city
	if(LOAD_VERSION < V_0_14_1)
	{
		for(Location* loc : locations)
		{
			if(loc && loc->type == L_CAMP)
			{
				Location* city = GetNearestSettlement(loc->pos);
				if(Vec2::Distance(loc->pos, city->pos) < 16.f)
					loc->pos.y += 16.f;
			}
		}
	}

	f >> tiles;

	if(currentLocationIndex != -1)
		currentLocation = locations[currentLocationIndex];
	else
		currentLocation = nullptr;
	gameLevel->locationIndex = currentLocationIndex;
	gameLevel->location = currentLocation;
	if(gameLevel->location && Any(state, State::INSIDE_LOCATION, State::INSIDE_ENCOUNTER))
		gameLevel->Apply();
}

//=================================================================================================
void World::LoadNews(GameReader& f)
{
	uint count;
	f >> count;
	news.resize(count);
	for(News*& n : news)
	{
		n = new News;
		f >> n->addTime;
		f.ReadString2(n->text);
	}
}

//=================================================================================================
void World::Write(BitStreamWriter& f)
{
	f << worldSize;
	WriteTime(f);

	// locations
	f.WriteCasted<byte>(locations.size());
	for(Location* locPtr : locations)
	{
		if(!locPtr)
		{
			f.WriteCasted<byte>(L_NULL);
			continue;
		}

		Location& loc = *locPtr;
		f.WriteCasted<byte>(loc.type);
		if(loc.type == L_DUNGEON)
			f.WriteCasted<byte>(loc.GetLastLevel() + 1);
		else if(loc.type == L_CITY)
		{
			City& city = (City&)loc;
			f.WriteCasted<byte>(city.citizens);
			f.WriteCasted<word>(city.citizensWorld);
		}
		f.WriteCasted<byte>(loc.state);
		f.WriteCasted<byte>(loc.target);
		f << loc.pos;
		f << loc.name;
		f.WriteCasted<byte>(loc.image);
	}
	f.WriteCasted<byte>(currentLocationIndex);
	f << worldPos;

	// position on world map when inside encounter location
	if(state == State::INSIDE_ENCOUNTER)
	{
		f << true;
		f << travelLocation;
		f << travelStartPos;
		f << travelTargetPos;
		f << travelTimer;
	}
	else
		f << false;
}

//=================================================================================================
void World::WriteTime(BitStreamWriter& f)
{
	f << worldtime;
	f << date;
}

//=================================================================================================
bool World::Read(BitStreamReader& f)
{
	f >> worldSize;
	ReadTime(f);
	if(!f)
	{
		Error("Read world: Broken packet for world time.");
		return false;
	}

	// count of locations
	byte count;
	f >> count;
	if(!f.Ensure(count))
	{
		Error("Read world: Broken packet for locations.");
		return false;
	}

	// locations
	locations.resize(count);
	uint index = 0;
	for(Location*& loc : locations)
	{
		LOCATION type;
		f.ReadCasted<byte>(type);
		if(!f)
		{
			Error("Read world: Broken packet for location %u.", index);
			return false;
		}

		if(type == L_NULL)
		{
			loc = nullptr;
			++index;
			continue;
		}

		switch(type)
		{
		case L_DUNGEON:
			{
				byte levels;
				f >> levels;
				if(!f)
				{
					Error("Read world: Broken packet for dungeon location %u.", index);
					return false;
				}
				else if(levels == 0)
				{
					Error("Read world: Location %d with 0 levels.", index);
					return false;
				}

				if(levels == 1)
					loc = new SingleInsideLocation;
				else
					loc = new MultiInsideLocation(levels);
			}
			break;
		case L_CAVE:
			loc = new Cave;
			break;
		case L_OUTSIDE:
		case L_ENCOUNTER:
		case L_CAMP:
			loc = new OutsideLocation;
			break;
		case L_CITY:
			{
				byte citizens;
				word worldCitizens;
				f >> citizens;
				f >> worldCitizens;
				if(!f)
				{
					Error("Read world: Broken packet for city location %u.", index);
					return false;
				}

				City* city = new City;
				loc = city;
				city->citizens = citizens;
				city->citizensWorld = worldCitizens;
			}
			break;
		case L_OFFSCREEN:
			loc = new OffscreenLocation;
			break;
		default:
			Error("Read world: Unknown location type %d for location %u.", type, index);
			return false;
		}

		// location data
		loc->index = index;
		f.ReadCasted<byte>(loc->state);
		f.ReadCasted<byte>(loc->target);
		f >> loc->pos;
		f >> loc->name;
		f.ReadCasted<byte>(loc->image);
		if(!f)
		{
			Error("Read world: Broken packet(2) for location %u.", index);
			return false;
		}
		loc->type = type;

		++index;
	}

	// current location
	f.ReadCasted<byte>(currentLocationIndex);
	f >> worldPos;
	if(!f)
	{
		Error("Read world: Broken packet for current location.");
		return false;
	}
	if(currentLocationIndex == 255)
	{
		currentLocationIndex = -1;
		currentLocation = nullptr;
		gameLevel->locationIndex = -1;
		gameLevel->location = nullptr;
	}
	else
	{
		if(currentLocationIndex >= (int)locations.size() || !locations[currentLocationIndex])
		{
			Error("Read world: Invalid location %d.", currentLocationIndex);
			return false;
		}
		currentLocation = locations[currentLocationIndex];
		gameLevel->locationIndex = currentLocationIndex;
		gameLevel->location = currentLocation;
	}

	// position on world map when inside encounter locations
	bool insideEncounter;
	f >> insideEncounter;
	if(!f)
	{
		Error("Read world: Broken packet for in travel data.");
		return false;
	}
	if(insideEncounter)
	{
		state = State::INSIDE_ENCOUNTER;
		f >> travelLocation;
		f >> travelStartPos;
		f >> travelTargetPos;
		f >> travelTimer;
		if(!f)
		{
			Error("Read world: Broken packet for in travel data (2).");
			return false;
		}
	}

	return true;
}

//=================================================================================================
void World::ReadTime(BitStreamReader& f)
{
	f >> worldtime;
	f >> date;
}

//=================================================================================================
bool World::IsSameWeek(int worldtime2) const
{
	if(worldtime2 == -1)
		return false;
	int week = worldtime / 10;
	int week2 = worldtime2 / 10;
	return week == week2;
}

//=================================================================================================
cstring World::GetDate() const
{
	return Format(txDate, date.day + 1, txMonth[date.month], date.year);
}

//=================================================================================================
cstring World::GetDate(const Date& date) const
{
	return Format(txDate, date.day + 1, txMonth[date.month], date.year);
}

//=================================================================================================
void World::SetStartDate(const Date& date)
{
	if(!startup)
		throw ScriptException("Start date can only be set at startup.");
	this->date = date;
	startDate = date;
}

//=================================================================================================
Location* World::GetLocationByType(LOCATION type, int target) const
{
	for(Location* loc : locations)
	{
		if(loc && loc->type == type && (target == ANY_TARGET || loc->target == target))
			return loc;
	}
	return nullptr;
}

//=================================================================================================
Location* World::GetRandomSettlement(Location* excluded) const
{
	int index = Rand() % settlements;
	if(locations[index] == excluded)
		index = (index + 1) % settlements;
	return locations[index];
}

//=================================================================================================
Location* World::GetRandomFreeSettlement(Location* excluded) const
{
	int index = Rand() % settlements;
	int startIndex = index;

	do
	{
		Location* loc = locations[index];
		if(loc != excluded && !loc->activeQuest)
			return loc;
		index = (index + 1) % settlements;
	}
	while(index != startIndex);

	return nullptr;
}

//=================================================================================================
Location* World::GetRandomCity(Location* excluded) const
{
	int count = 0;
	while(LocationHelper::IsCity(locations[count]))
		++count;

	int index = Rand() % count;
	if(locations[index] == excluded)
		index = (index + 1) % count;

	return locations[index];
}

//=================================================================================================
// Get random settlement excluded from used list
Location* World::GetRandomSettlement(vector<Location*>& used, int target) const
{
	int index = Rand() % settlements;
	int startIndex = index;

	while(true)
	{
		Location* loc = locations[index];

		if(target != ANY_TARGET && loc->target != target)
		{
			index = (index + 1) % settlements;
			continue;
		}

		bool ok = true;
		for(vector<Location*>::const_iterator it = used.begin(), end = used.end(); it != end; ++it)
		{
			if(*it == loc)
			{
				index = (index + 1) % settlements;
				ok = false;
				break;
			}
		}

		if(ok)
		{
			used.push_back(loc);
			return loc;
		}
		else if(index == startIndex)
			throw "Out of free cities!";
	}
}

//=================================================================================================
Location* World::GetClosestLocation(LOCATION type, const Vec2& pos, int target, int flags)
{
	Location* best = nullptr;
	int index = 0;
	float dist, bestDist;
	const bool allowActive = IsSet(flags, F_ALLOW_ACTIVE);
	const bool excluded = IsSet(flags, F_EXCLUDED);

	for(vector<Location*>::iterator it = locations.begin(), end = locations.end(); it != end; ++it, ++index)
	{
		Location* loc = *it;
		if(!loc || loc->type != type)
			continue;
		if(!allowActive && loc->activeQuest)
			continue;
		if(target != ANY_TARGET)
		{
			bool ok = (loc->target == target);
			if(ok == excluded)
				continue;
		}
		dist = Vec2::Distance(loc->pos, pos);
		if(!best || dist < bestDist)
		{
			best = loc;
			bestDist = dist;
		}
	}

	return best;
}

//=================================================================================================
Location* World::GetClosestLocation(LOCATION type, const Vec2& pos, const int* targets, int targetsCount, int flags)
{
	Location* best = nullptr;
	int index = 0;
	float dist, bestDist;
	const bool allowActive = IsSet(flags, F_ALLOW_ACTIVE);
	const bool excluded = IsSet(flags, F_EXCLUDED);

	for(vector<Location*>::iterator it = locations.begin(), end = locations.end(); it != end; ++it, ++index)
	{
		Location* loc = *it;
		if(!loc || loc->type != type)
			continue;
		if(!allowActive && loc->activeQuest)
			continue;
		bool ok = false;
		for(int i = 0; i < targetsCount; ++i)
		{
			if(loc->target == targets[i])
			{
				ok = true;
				break;
			}
		}
		if(ok == excluded)
			continue;
		dist = Vec2::Distance(loc->pos, pos);
		if(!best || dist < bestDist)
		{
			best = loc;
			bestDist = dist;
		}
	}

	return best;
}

//=================================================================================================
Location* World::GetClosestLocationArrayS(LOCATION type, const Vec2& pos, CScriptArray* array)
{
	Location* loc = GetClosestLocation(type, pos, (int*)array->GetBuffer(), array->GetSize(), 0);
	array->Release();
	return loc;
}

//=================================================================================================
bool World::TryFindPlace(Vec2& pos, float range, bool allowExact)
{
	Vec2 pt;

	if(allowExact)
		pt = pos;
	else
		pt = (pos + Vec2::RandomCirclePt(range)).Clamped(Vec2(worldBounds.x, worldBounds.x), Vec2(worldBounds.y, worldBounds.y));

	for(int i = 0; i < 20; ++i)
	{
		bool valid = true;
		for(Location* loc : locations)
		{
			if(loc && Vec2::Distance(pt, loc->pos) < 32)
			{
				valid = false;
				break;
			}
		}

		if(valid)
		{
			pos = pt;
			return true;
		}
		else
			pt = (pos + Vec2::RandomCirclePt(range)).Clamped(Vec2(worldBounds.x, worldBounds.x), Vec2(worldBounds.y, worldBounds.y));
	}

	return false;
}

//=================================================================================================
Vec2 World::FindPlace(const Vec2& pos, float range, bool allowExact)
{
	Vec2 pt;

	if(allowExact)
		pt = pos;
	else
		pt = (pos + Vec2::RandomCirclePt(range)).Clamped(Vec2(worldBounds.x, worldBounds.x), Vec2(worldBounds.y, worldBounds.y));

	for(int i = 0; i < 20; ++i)
	{
		bool valid = true;
		for(Location* loc : locations)
		{
			if(loc && Vec2::Distance(pt, loc->pos) < 32)
			{
				valid = false;
				break;
			}
		}

		if(valid)
			break;

		pt = (pos + Vec2::RandomCirclePt(range)).Clamped(Vec2(worldBounds.x, worldBounds.x), Vec2(worldBounds.y, worldBounds.y));
	}

	return pt;
}

//=================================================================================================
Vec2 World::FindPlace(const Vec2& pos, float minRange, float maxRange)
{
	for(int i = 0; i < 20; ++i)
	{
		float range = Random(minRange, maxRange);
		float angle = Random(PI * 2);
		Vec2 pt = pos + Vec2(sin(angle) * range, cos(angle) * range);

		bool valid = true;
		for(Location* loc : locations)
		{
			if(loc && Vec2::Distance(pt, loc->pos) < 32)
			{
				valid = false;
				break;
			}
		}

		if(valid)
			return pt;
	}

	return pos;
}

//=================================================================================================
Vec2 World::FindPlace(const Box2d& box)
{
	Vec2 pos;
	for(int i = 0; i < 20; ++i)
	{
		pos = box.GetRandomPoint();

		bool valid = true;
		for(Location* loc : locations)
		{
			if(loc && Vec2::Distance(pos, loc->pos) < 32)
			{
				valid = false;
				break;
			}
		}

		if(valid)
			break;
	}
	return pos;
}

//=================================================================================================
Vec2 World::GetRandomPlace()
{
	const Vec2 pos = Vec2::Random(worldBounds.x, worldBounds.y);
	return FindPlace(pos, 64.f);
}

//=================================================================================================
// Searches for location with selected spawn group
// If cleared respawns enemies
// If nothing found creates camp
Location* World::GetRandomSpawnLocation(const Vec2& pos, UnitGroup* group, float range)
{
	Location* bestOk = nullptr, *bestEmpty = nullptr;
	int index = settlements;
	float okRange, emptyRange, dist;

	for(vector<Location*>::iterator it = locations.begin() + settlements, end = locations.end(); it != end; ++it, ++index)
	{
		if(!*it)
			continue;
		if(!(*it)->activeQuest && (*it)->type == L_DUNGEON)
		{
			InsideLocation* inside = (InsideLocation*)*it;
			if((*it)->state == LS_CLEARED || inside->group == group || !inside->group)
			{
				dist = Vec2::Distance(pos, (*it)->pos);
				if(dist <= range)
				{
					if(inside->group == group)
					{
						if(!bestOk || dist < okRange)
						{
							bestOk = inside;
							okRange = dist;
						}
					}
					else
					{
						if(!bestEmpty || dist < emptyRange)
						{
							bestEmpty = inside;
							emptyRange = dist;
						}
					}
				}
			}
		}
	}

	if(bestOk)
	{
		bestOk->reset = true;
		return bestOk;
	}

	if(bestEmpty)
	{
		bestEmpty->group = group;
		bestEmpty->reset = true;
		return bestEmpty;
	}

	const Vec2 targetPos = FindPlace(pos, range / 2);
	return CreateCamp(targetPos, group);
}

//=================================================================================================
City* World::GetRandomSettlement(delegate<bool(City*)> pred)
{
	int startIndex = Rand() % settlements;
	int index = startIndex;
	do
	{
		City* loc = static_cast<City*>(locations[index]);
		if(pred(loc))
			return loc;
		index = (index + 1) % settlements;
	}
	while(index != startIndex);
	return nullptr;
}

//=================================================================================================
Location* World::GetRandomSettlementWeighted(delegate<float(Location*)> func)
{
	float bestValue = -1.f;
	int bestIndex = -1;
	int startIndex = Rand() % settlements;
	int index = startIndex;
	do
	{
		Location* loc = locations[index];
		float result = func(loc);
		if(result >= 0.f && result > bestValue)
		{
			bestValue = result;
			bestIndex = index;
		}
		index = (index + 1) % settlements;
	}
	while(index != startIndex);
	return (bestIndex != -1 ? locations[bestIndex] : nullptr);
}

//=================================================================================================
Location* World::GetRandomLocation(delegate<bool(Location*)> pred)
{
	int startIndex = Rand() % locations.size();
	int index = startIndex;
	do
	{
		Location* loc = locations[index];
		if(loc && pred(loc))
			return loc;
		index = (index + 1) % locations.size();
	}
	while(index != startIndex);
	return nullptr;
}

//=================================================================================================
void World::ExitToMap()
{
	if(state == State::INSIDE_ENCOUNTER)
	{
		state = State::TRAVEL;
		currentLocationIndex = -1;
		currentLocation = nullptr;
		gameLevel->locationIndex = -1;
		gameLevel->location = nullptr;
	}
	else
		state = State::ON_MAP;
}

//=================================================================================================
void World::ChangeLevel(int index, bool encounter)
{
	state = encounter ? State::INSIDE_ENCOUNTER : State::INSIDE_LOCATION;
	currentLocationIndex = index;
	currentLocation = locations[index];
	gameLevel->locationIndex = currentLocationIndex;
	gameLevel->location = currentLocation;
}

//=================================================================================================
void World::StartInLocation(Location* loc)
{
	locations.push_back(loc);
	currentLocationIndex = locations.size() - 1;
	currentLocation = loc;
	state = State::INSIDE_LOCATION;
	loc->index = currentLocationIndex;
	gameLevel->locationIndex = currentLocationIndex;
	gameLevel->location = loc;
	gameLevel->isOpen = true;
	startup = false;
}

//=================================================================================================
void World::StartEncounter()
{
	state = State::INSIDE_ENCOUNTER;
	currentLocation = encounterLoc;
	currentLocationIndex = encounterLoc->index;
	gameLevel->locationIndex = currentLocationIndex;
	gameLevel->location = currentLocation;
}

//=================================================================================================
void World::Travel(int index, bool order)
{
	if(state == State::TRAVEL)
		return;

	// leave current location
	if(gameLevel->isOpen)
	{
		game->LeaveLocation();
		gameLevel->isOpen = false;
	}

	state = State::TRAVEL;
	currentLocation = nullptr;
	currentLocationIndex = -1;
	gameLevel->location = nullptr;
	gameLevel->locationIndex = -1;
	travelTimer = 0.f;
	travelStartPos = worldPos;
	travelLocation = locations[index];
	travelTargetPos = travelLocation->pos;
	travelDir = AngleLH(worldPos.x, worldPos.y, travelTargetPos.x, travelTargetPos.y);
	if(Net::IsLocal())
		travelFirstFrame = true;

	if(Net::IsServer() || (Net::IsClient() && order))
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::TRAVEL;
		c.id = index;
	}
}

//=================================================================================================
void World::TravelPos(const Vec2& pos, bool order)
{
	if(state == State::TRAVEL)
		return;

	// leave current location
	if(gameLevel->isOpen)
	{
		game->LeaveLocation();
		gameLevel->isOpen = false;
	}

	state = State::TRAVEL;
	currentLocation = nullptr;
	currentLocationIndex = -1;
	gameLevel->location = nullptr;
	gameLevel->locationIndex = -1;
	travelTimer = 0.f;
	travelStartPos = worldPos;
	travelLocation = nullptr;
	travelTargetPos = pos;
	travelDir = AngleLH(worldPos.x, worldPos.y, travelTargetPos.x, travelTargetPos.y);
	if(Net::IsLocal())
		travelFirstFrame = true;

	if(Net::IsServer() || (Net::IsClient() && order))
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::TRAVEL_POS;
		c.pos.x = pos.x;
		c.pos.y = pos.y;
	}
}

//=================================================================================================
void World::UpdateTravel(float dt)
{
	if(travelFirstFrame)
	{
		travelFirstFrame = false;
		return;
	}

	dayTimer += dt;
	if(dayTimer >= 1.f)
	{
		// another day passed
		--dayTimer;
		if(Net::IsLocal())
			Update(1, UM_TRAVEL);
	}

	float dist = Vec2::Distance(travelStartPos, travelTargetPos);
	travelTimer += dt;
	if(travelTimer * 3 >= dist / TRAVEL_SPEED)
	{
		// end of travel
		if(Net::IsLocal())
		{
			team->OnTravel(Vec2::Distance(worldPos, travelTargetPos));
			EndTravel();
		}
		else
			worldPos = travelTargetPos;
	}
	else
	{
		Vec2 dir = travelTargetPos - travelStartPos;
		float travelDist = travelTimer / dist * TRAVEL_SPEED * 3;
		worldPos = travelStartPos + dir * travelDist;
		if(Net::IsLocal())
			team->OnTravel(travelDist);

		// reveal nearby locations, check encounters
		revealTimer += dt;
		if(Net::IsLocal() && revealTimer >= 0.25f)
		{
			revealTimer = 0;

			UnitGroup* group = nullptr;
			for(Location* ploc : locations)
			{
				if(!ploc)
					continue;
				Location& loc = *ploc;
				float dist = Vec2::Distance(worldPos, loc.pos);
				if(dist <= 50.f)
				{
					if(loc.state != LS_CLEARED && dist <= 32.f && loc.group && loc.group->encounterChance != 0)
					{
						group = loc.group;
						int chance = loc.group->encounterChance;
						if(loc.type == L_CAMP)
							chance *= 2;
						encounterChance += chance;
					}

					if(loc.state != LS_HIDDEN)
						loc.SetKnown();
				}
			}

			int enc = -1, index = -1;
			for(Encounter* encounter : encounters)
			{
				++index;
				if(!encounter)
					continue;
				if(Vec2::Distance(encounter->pos, worldPos) < encounter->range)
				{
					if(!encounter->checkFunc || encounter->checkFunc())
					{
						encounterChance += encounter->chance;
						enc = index;
					}
				}
			}

			encounterChance += 1 + globalEncounters.size();

			if(Rand() % 500 < ((int)encounterChance) - 25 || (gui->HaveFocus() && GKey.DebugKey(Key::E)))
			{
				encounterChance = 0.f;
				StartEncounter(enc, group);
			}
		}
	}
}

//=================================================================================================
void World::StartEncounter(int enc, UnitGroup* group)
{
	state = State::ENCOUNTER;
	if(Net::IsOnline())
		Net::PushChange(NetChange::UPDATE_MAP_POS);

	encounter.st = GetTileSt(worldPos);
	if(encounter.st < 3)
		encounter.st = 3;

	cstring text;

	if(enc != -1)
	{
		// quest encounter
		encounter.mode = ENCOUNTER_QUEST;
		encounter.encounter = GetEncounter(enc);
		text = encounter.encounter->text;
	}
	else
	{
		GlobalEncounter* globalEnc = nullptr;
		for(GlobalEncounter* enc : globalEncounters)
		{
			if(Rand() % 100 < enc->chance)
			{
				globalEnc = enc;
				break;
			}
		}

		if(globalEnc)
		{
			encounter.mode = ENCOUNTER_GLOBAL;
			encounter.global = globalEnc;
			text = globalEnc->text;
		}
		else
		{
			bool special = false;
			if(!group)
			{
				if(Rand() % 6 == 0)
					group = UnitGroup::Get("bandits");
				else
					special = true;
			}
			else if(Rand() % 3 == 0)
				special = true;

			if(special || GKey.DebugKey(Key::Shift))
			{
				// special encounter
				encounter.mode = ENCOUNTER_SPECIAL;
				encounter.special = (SpecialEncounter)(Rand() % SE_MAX_NORMAL);
				if(IsDebug() && input->Focus())
				{
					if(input->Down(Key::I))
						encounter.special = SE_CRAZY_HEROES;
					else if(input->Down(Key::B))
						encounter.special = SE_BANDITS_VS_TRAVELERS;
					else if(input->Down(Key::C))
						encounter.special = SE_CRAZY_COOK;
				}

				if((encounter.special == SE_CRAZY_MAGE || encounter.special == SE_CRAZY_HEROES) && Rand() % 10 == 0)
					encounter.special = SE_CRAZY_COOK;

				switch(encounter.special)
				{
				default:
					assert(0);
				case SE_CRAZY_MAGE:
					text = txEncCrazyMage;
					break;
				case SE_CRAZY_HEROES:
					text = txEncCrazyHeroes;
					break;
				case SE_MERCHANT:
					text = txEncMerchant;
					break;
				case SE_HEROES:
					if(!tomirSpawned && Rand() % 5 == 0)
					{
						encounter.special = SE_TOMIR;
						tomirSpawned = true;
						text = txEncSingleHero;
					}
					else
						text = txEncHeroes;
					break;
				case SE_BANDITS_VS_TRAVELERS:
					text = txEncBanditsAttackTravelers;
					break;
				case SE_HEROES_VS_ENEMIES:
					text = txEncHeroesAttack;
					break;
				case SE_CRAZY_COOK:
					text = txEncCrazyCook;
					break;
				case SE_ENEMIES_COMBAT:
					text = txEncEnemiesCombat;
					break;
				}
			}
			else
			{
				// combat encounter
				encounter.mode = ENCOUNTER_COMBAT;
				encounter.group = group;
				text = group->encounterText.c_str();
				if(group->isList)
				{
					for(UnitGroup::Entry& entry : group->entries)
					{
						if(entry.isLeader)
						{
							encounter.group = entry.group;
							break;
						}
					}
				}
			}
		}
	}

	gui->ShowEncounterMessage(text);
}

//=================================================================================================
void World::StopTravel(const Vec2& pos, bool send)
{
	if(state != State::TRAVEL)
		return;
	state = State::ON_MAP;
	if(Net::IsOnline() && send)
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::STOP_TRAVEL;
		c.pos.x = pos.x;
		c.pos.y = pos.y;
	}
}

//=================================================================================================
void World::EndTravel()
{
	if(state != State::TRAVEL)
		return;
	state = State::ON_MAP;
	if(travelLocation)
	{
		currentLocation = travelLocation;
		currentLocationIndex = travelLocation->index;
		worldPos = currentLocation->pos;
		travelLocation = nullptr;
		gameLevel->locationIndex = currentLocationIndex;
		gameLevel->location = currentLocation;
	}
	else
		worldPos = travelTargetPos;
	if(Net::IsServer())
		Net::PushChange(NetChange::END_TRAVEL);
}

//=================================================================================================
void World::Warp(int index, bool order)
{
	if(gameLevel->isOpen)
	{
		game->LeaveLocation(false, false);
		gameLevel->isOpen = false;
	}

	currentLocationIndex = index;
	currentLocation = locations[currentLocationIndex];
	worldPos = currentLocation->pos;
	gameLevel->locationIndex = currentLocationIndex;
	gameLevel->location = currentLocation;

	if(Net::IsServer() || (Net::IsClient() && order))
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::CHEAT_TRAVEL;
		c.id = index;
	}
}

//=================================================================================================
void World::WarpPos(const Vec2& pos, bool order)
{
	if(gameLevel->isOpen)
	{
		game->LeaveLocation(false, false);
		gameLevel->isOpen = false;
	}

	worldPos = pos;
	currentLocationIndex = -1;
	currentLocation = nullptr;
	gameLevel->locationIndex = -1;
	gameLevel->location = nullptr;

	if(Net::IsServer() || (Net::IsClient() && order))
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::CHEAT_TRAVEL_POS;
		c.pos.x = pos.x;
		c.pos.y = pos.y;
	}
}

//=================================================================================================
Encounter* World::AddEncounter(int& index, Quest* quest)
{
	Encounter* enc;
	if(quest && quest->type == Q_SCRIPTED)
		enc = new Encounter(quest);
	else
		enc = new Encounter;
	enc->group = UnitGroup::empty;

	for(int i = 0, size = (int)encounters.size(); i < size; ++i)
	{
		if(!encounters[i])
		{
			index = i;
			enc->index = index;
			encounters[i] = enc;
			return enc;
		}
	}

	index = encounters.size();
	enc->index = index;
	encounters.push_back(enc);
	return enc;
}

//=================================================================================================
Encounter* World::AddEncounterS(Quest* quest)
{
	int index;
	return AddEncounter(index, quest);
}

//=================================================================================================
void World::RemoveEncounter(int index)
{
	assert(InRange(index, 0, (int)encounters.size() - 1) && encounters[index]);
	delete encounters[index];
	encounters[index] = nullptr;
}

//=================================================================================================
void World::RemoveEncounter(Quest* quest)
{
	assert(quest);
	for(uint i = 0; i < encounters.size(); ++i)
	{
		Encounter* enc = encounters[i];
		if(enc && enc->quest == quest)
		{
			delete enc;
			encounters[i] = nullptr;
		}
	}
}

//=================================================================================================
void World::RemoveGlobalEncounter(Quest* quest)
{
	LoopAndRemove(globalEncounters, [=](GlobalEncounter* globalEnc)
	{
		if(globalEnc->quest == quest)
		{
			delete globalEnc;
			return true;
		}
		return false;
	});
}

//=================================================================================================
Encounter* World::GetEncounter(int index)
{
	assert(InRange(index, 0, (int)encounters.size() - 1) && encounters[index]);
	return encounters[index];
}

//=================================================================================================
Encounter* World::RecreateEncounter(int index)
{
	assert(InRange(index, 0, (int)encounters.size() - 1));
	Encounter* e = new Encounter;
	e->index = index;
	encounters[index] = e;
	return e;
}

//=================================================================================================
Encounter* World::RecreateEncounterS(Quest* quest, int index)
{
	assert(InRange(index, 0, (int)encounters.size() - 1));
	Encounter* e = new Encounter(quest);
	e->index = index;
	encounters[index] = e;
	return e;
}

//=================================================================================================
// Set unit pos, dir according to travelDir
void World::GetOutsideSpawnPoint(Vec3& pos, float& dir) const
{
	const float mapSize = 256.f;
	const float dist = 40.f;

	// reverse dir, if you exit from north then you enter from south
	float entryDir = Clip(travelDir + PI);

	if(entryDir < PI / 4 || entryDir > 7.f / 4 * PI)
	{
		// east
		dir = PI / 2;
		if(entryDir < PI / 4)
			pos = Vec3(mapSize - dist, 0, Lerp(mapSize / 2, dist, entryDir / (PI / 4)));
		else
			pos = Vec3(mapSize - dist, 0, Lerp(mapSize - dist, mapSize / 2, (entryDir - 7.f / 4 * PI) / (PI / 4)));
	}
	else if(entryDir < 3.f / 4 * PI)
	{
		// south
		dir = PI;
		pos = Vec3(Lerp(mapSize - dist, dist, (entryDir - 1.f / 4 * PI) / (PI / 2)), 0, dist);
	}
	else if(entryDir < 5.f / 4 * PI)
	{
		// west
		dir = 3.f / 2 * PI;
		pos = Vec3(dist, 0, Lerp(dist, mapSize - dist, (entryDir - 3.f / 4 * PI) / (PI / 2)));
	}
	else
	{
		// north
		dir = 0;
		pos = Vec3(Lerp(dist, mapSize - dist, (entryDir - 5.f / 4 * PI) / (PI / 2)), 0, mapSize - dist);
	}
}

//=================================================================================================
float World::GetTravelDays(float dist)
{
	return dist * World::MAP_KM_RATIO / World::TRAVEL_SPEED;
}

//=================================================================================================
// Set travelDir using unit pos
// When crossed travel bariers simply calculate angle
// When using city gates find closest line segment point and then use above calculation on this point
void World::SetTravelDir(const Vec3& pos)
{
	const float mapSize = 256.f;
	const float border = 33.f;
	Vec2 unitPos = pos.XZ();

	if(Box2d(border, border, mapSize - border, mapSize - border).IsInside(unitPos))
	{
		// not inside exit border, find closest line segment
		const float mini = 32.f;
		const float maxi = 256 - 32.f;
		float bestDist = 999.f, dist;
		Vec2 pt, closePt;
		// check right
		dist = GetClosestPointOnLineSegment(Vec2(maxi, mini), Vec2(maxi, maxi), unitPos, pt);
		if(dist < bestDist)
		{
			bestDist = dist;
			closePt = pt;
		}
		// check left
		dist = GetClosestPointOnLineSegment(Vec2(mini, mini), Vec2(mini, maxi), unitPos, pt);
		if(dist < bestDist)
		{
			bestDist = dist;
			closePt = pt;
		}
		// check bottom
		dist = GetClosestPointOnLineSegment(Vec2(mini, mini), Vec2(maxi, mini), unitPos, pt);
		if(dist < bestDist)
		{
			bestDist = dist;
			closePt = pt;
		}
		// check top
		dist = GetClosestPointOnLineSegment(Vec2(mini, maxi), Vec2(maxi, maxi), unitPos, pt);
		if(dist < bestDist)
			closePt = pt;
		unitPos = closePt;
	}

	// point is outside of location borders
	if(unitPos.x < border)
		travelDir = Lerp(3.f / 4.f * PI, 5.f / 4.f * PI, 1.f - (unitPos.y - border) / (mapSize - border * 2));
	else if(unitPos.x > mapSize - border)
	{
		if(unitPos.y > mapSize / 2)
			travelDir = Lerp(0.f, 1.f / 4 * PI, (unitPos.y - mapSize / 2) / (mapSize - mapSize / 2 - border));
		else
			travelDir = Lerp(7.f / 4 * PI, PI * 2, (unitPos.y - border) / (mapSize - mapSize / 2 - border));
	}
	else if(unitPos.y < border)
		travelDir = Lerp(5.f / 4 * PI, 7.f / 4 * PI, (unitPos.x - border) / (mapSize - border * 2));
	else
		travelDir = Lerp(1.f / 4 * PI, 3.f / 4 * PI, 1.f - (unitPos.x - border) / (mapSize - border * 2));

	// convert RH angle to LH and then entry to exit dir
	travelDir = Clip(ConvertAngle(travelDir) + PI);
}

//=================================================================================================
// Mark all locations as known
void World::Reveal()
{
	for(Location* loc : locations)
	{
		if(loc)
			loc->SetKnown();
	}
}

//=================================================================================================
void World::AddNews(cstring text)
{
	assert(text);

	News* n = new News;
	n->text = text;
	n->addTime = worldtime;

	news.push_back(n);
}

//=================================================================================================
void World::DeleteCamp(Camp* camp, bool remove)
{
	assert(camp);

	int index = camp->index;

	if(Net::IsOnline())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::REMOVE_CAMP;
		c.id = index;
	}

	delete camp;

	if(remove)
		RemoveLocation(index);
}

//=================================================================================================
void World::AbadonLocation(Location* loc)
{
	assert(loc);

	// only works for OutsideLocation or Cave for now!
	assert((loc->outside && loc->type != L_CITY) || loc->type == L_CAVE);

	LocationPart* locPart;
	if(loc->type == L_CAVE)
		locPart = static_cast<Cave*>(loc);
	else
		locPart = static_cast<OutsideLocation*>(loc);

	// remove camp in 4-8 days
	if(loc->type == L_CAMP)
	{
		Camp* camp = static_cast<Camp*>(loc);
		camp->createTime = world->GetWorldtime() - 30 + Random(4, 8);

		// turn off lights
		if(loc != currentLocation)
		{
			BaseObject* campfire = BaseObject::Get("campfire"),
				*campfireOff = BaseObject::Get("campfire_off"),
				*torch = BaseObject::Get("torch"),
				*torchOff = BaseObject::Get("torch_off");
			for(Object* obj : camp->objects)
			{
				if(obj->base == campfire)
					obj->base = campfireOff;
				else if(obj->base == torch)
					obj->base = torchOff;
			}
		}
	}

	// if location is open
	if(loc == currentLocation)
	{
		// remove units
		for(Unit*& u : locPart->units)
		{
			if(u->IsAlive() && game->pc->unit->IsEnemy(*u))
			{
				u->toRemove = true;
				gameLevel->toRemove.push_back(u);
			}
		}

		// remove items from chests
		for(Chest* chest : locPart->chests)
		{
			if(!chest->GetUser())
				chest->items.clear();
		}
	}
	else
	{
		// delete units
		DeleteElements(locPart->units, [](Unit* unit) { return unit->IsAlive() && game->pc->unit->IsEnemy(*unit); });

		// remove items from chests
		for(Chest* chest : locPart->chests)
			chest->items.clear();
	}

	loc->group = UnitGroup::empty;
	if(loc->lastVisit != -1)
		loc->lastVisit = worldtime;
}

//=================================================================================================
void World::VerifyObjects()
{
	int errors = 0, e;

	for(Location* l : locations)
	{
		if(!l || l->type == L_OFFSCREEN)
			continue;
		if(l->outside)
		{
			OutsideLocation* outside = static_cast<OutsideLocation*>(l);
			e = 0;
			VerifyObjects(outside->objects, e);
			if(e > 0)
			{
				Error("%d errors in outside location '%s'.", e, outside->name.c_str());
				errors += e;
			}
			if(l->type == L_CITY)
			{
				City* city = static_cast<City*>(outside);
				for(InsideBuilding* ib : city->insideBuildings)
				{
					e = 0;
					VerifyObjects(ib->objects, e);
					if(e > 0)
					{
						Error("%d errors in city '%s', building '%s'.", e, city->name.c_str(), ib->building->id.c_str());
						errors += e;
					}
				}
			}
		}
		else
		{
			InsideLocation* inside = static_cast<InsideLocation*>(l);
			if(inside->IsMultilevel())
			{
				MultiInsideLocation* m = static_cast<MultiInsideLocation*>(inside);
				int index = 1;
				for(InsideLocationLevel* lvl : m->levels)
				{
					e = 0;
					VerifyObjects(lvl->objects, e);
					if(e > 0)
					{
						Error("%d errors in multi inside location '%s' at level %d.", e, m->name.c_str(), index);
						errors += e;
					}
					++index;
				}
			}
			else
			{
				SingleInsideLocation* s = static_cast<SingleInsideLocation*>(inside);
				e = 0;
				VerifyObjects(s->objects, e);
				if(e > 0)
				{
					Error("%d errors in single inside location '%s'.", e, s->name.c_str());
					errors += e;
				}
			}
		}
	}

	if(errors > 0)
		throw Format("Veryify objects failed with %d errors. Check log for details.", errors);
}

//=================================================================================================
void World::VerifyObjects(vector<Object*>& objects, int& errors)
{
	for(Object* o : objects)
	{
		if(!o->mesh && !o->base)
		{
			Error("Broken object at (%g,%g,%g).", o->pos.x, o->pos.y, o->pos.z);
			++errors;
		}
	}
}

//=================================================================================================
int& World::GetTileSt(const Vec2& pos)
{
	assert(pos.x >= 0 || pos.y >= 0 || pos.x < (float)worldSize || pos.y < (float)worldSize);
	Int2 tile(int(pos.x / TILE_SIZE), int(pos.y / TILE_SIZE));
	int ts = worldSize / TILE_SIZE;
	return tiles[tile.x + tile.y * ts];
}

//=================================================================================================
Unit* World::CreateUnit(UnitData* data, int level)
{
	assert(data);
	return offscreenLoc->CreateUnit(*data, level);
}
