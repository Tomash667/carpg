#include "Pch.h"
#include "GameCore.h"
#include "World.h"
#include "BitStreamFunc.h"
#include "Language.h"
#include "LocationHelper.h"
#include "GameFile.h"
#include "SaveState.h"
#include "LoadingHandler.h"
#include "ScriptManager.h"
#include "Var.h"
#include "Level.h"
#include "Camp.h"
#include "Cave.h"
#include "City.h"
#include "MultiInsideLocation.h"
#include "Net.h"
#include "Encounter.h"
#include "Quest.h"
#include "News.h"
#include "Team.h"
#include "QuestManager.h"
#include "Quest_Crazies.h"
#include "Quest_Mages.h"
#include "Quest_Scripted.h"
#include "Game.h"
#include "GameGui.h"
#include "WorldMapGui.h"

//-----------------------------------------------------------------------------
const float World::TRAVEL_SPEED = 28.f;
const float World::MAP_KM_RATIO = 1.f / 3; // 1200 pixels = 400 km
const int start_year = 100;
const float world_border = 50.f;
const int def_world_size = 1200;
World* global::world;
vector<string> txLocationStart, txLocationEnd;

// pre V_0_8 compatibility
namespace old
{
	const int def_world_size = 600;
}

//-----------------------------------------------------------------------------
// used temporary before deciding how many levels location should have (and use Single or MultiInsideLocation)
struct TmpLocation : public Location
{
	TmpLocation() : Location(false) {}

	void Apply(vector<std::reference_wrapper<LevelArea>>& areas) override {}
	void Write(BitStreamWriter& f) override {}
	bool Read(BitStreamReader& f) override { return false; }
	bool FindUnit(Unit*, int*) override { return false; }
	Unit* FindUnit(UnitData*, int&) override { return nullptr; }
};

//=================================================================================================
void World::LoadLanguage()
{
	txDate = Str("dateFormat");

	// encounters
	Language::Section s = Language::GetSection("Encounters");
	txEncCrazyMage = s.Get("encCrazyMage");
	txEncCrazyHeroes = s.Get("encCrazyHeroes");
	txEncCrazyCook = s.Get("encCrazyCook");
	txEncMerchant = s.Get("encMerchant");
	txEncHeroes = s.Get("encHeroes");
	txEncSingleHero = s.Get("encSingleHero");
	txEncBanditsAttackTravelers = s.Get("encBanditsAttackTravelers");
	txEncHeroesAttack = s.Get("encHeroesAttack");
	txEncGolem = s.Get("encGolem");
	txEncCrazy = s.Get("encCrazy");
	txEncUnk = s.Get("encUnk");
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
}

//=================================================================================================
void World::Init()
{
	gui = game_gui->world_map;
}

//=================================================================================================
void World::Cleanup()
{
	Reset();
}

//=================================================================================================
void World::OnNewGame()
{
	travel_location_index = -1;
	empty_locations = 0;
	create_camp = Random(5, 10);
	encounter_chance = 0.f;
	travel_dir = Random(MAX_ANGLE);
	year = start_year;
	day = 0;
	month = 0;
	worldtime = 0;
	day_timer = 0.f;
}

//=================================================================================================
void World::Reset()
{
	DeleteElements(locations);
	DeleteElements(encounters);
	DeleteElements(news);
}

//=================================================================================================
void World::Update(int days, UpdateMode mode)
{
	assert(days > 0);
	if(mode == UM_TRAVEL)
		assert(days == 1);

	UpdateDate(days);
	SpawnCamps(days);
	UpdateEncounters();
	if(Any(state, State::INSIDE_LOCATION, State::INSIDE_ENCOUNTER))
		game_level->Update();
	UpdateLocations();
	UpdateNews();
	quest_mgr->Update(days);

	if(Net::IsLocal())
		quest_mgr->UpdateQuests(days);

	if(mode == UM_TRAVEL)
		team->Update(1, true);
	else if(mode == UM_NORMAL)
		team->Update(days, false);

	// end of game
	if(year >= 160)
	{
		Info("Game over: you are too old.");
		game_gui->CloseAllPanels(true);
		game->end_of_game = true;
		game->death_fade = 0.f;
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
	day += days;
	if(day >= 30)
	{
		int count = day / 30;
		month += count;
		day -= count * 30;
		if(month >= 12)
		{
			count = month / 12;
			year += count;
			month -= count * 12;
		}
	}

	if(Net::IsOnline())
		Net::PushChange(NetChange::WORLD_TIME);
}

//=================================================================================================
void World::SpawnCamps(int days)
{
	create_camp -= days;
	if(create_camp <= 0)
	{
		create_camp = Random(5, 10);
		int index = Rand() % locations.size(),
			start = index;
		while(true)
		{
			Location* loc = locations[index];
			if(loc && loc->state != LS_CLEARED && loc->type == L_DUNGEON && loc->group->have_camps)
			{
				CreateCamp(loc->pos, loc->group, 128.f, false);
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
		if(!loc || loc->active_quest || loc->type == L_ENCOUNTER)
			continue;
		if(loc->type == L_CAMP)
		{
			Camp* camp = static_cast<Camp*>(loc);
			if(worldtime - camp->create_time >= 30
				&& current_location != loc // don't remove when team is inside
				&& (travel_location_index == -1 || locations[travel_location_index] != loc)) // don't remove when traveling to
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
					++empty_locations;
					*it = nullptr;
				}
			}
		}
		else if(loc->last_visit != -1 && worldtime - loc->last_visit >= 30)
		{
			loc->reset = true;
			if(loc->state == LS_CLEARED)
				loc->state = LS_ENTERED;
			if(loc->type == L_DUNGEON)
			{
				InsideLocation* inside = static_cast<InsideLocation*>(loc);
				inside->group = g_base_locations[inside->target].GetRandomGroup();
				if(inside->IsMultilevel())
					static_cast<MultiInsideLocation*>(inside)->Reset();
			}
		}
	}
}

//=================================================================================================
void World::UpdateNews()
{
	DeleteElements(news, [this](News* news) { return worldtime - news->add_time > 30; });
}

//=================================================================================================
int World::CreateCamp(const Vec2& pos, UnitGroup* group, float range, bool allow_exact)
{
	assert(group);

	Camp* loc = new Camp;
	loc->state = LS_UNKNOWN;
	loc->active_quest = nullptr;
	loc->type = L_CAMP;
	loc->last_visit = -1;
	loc->reset = false;
	loc->group = group;
	loc->pos = pos;
	loc->create_time = worldtime;
	SetLocationImageAndName(loc);

	FindPlaceForLocation(loc->pos, range, allow_exact);
	loc->st = GetTileSt(loc->pos);
	if(loc->st > 3)
		loc->st = 3;

	return AddLocation(loc);
}

//=================================================================================================
Location* World::CreateLocation(LOCATION type, int levels, int city_target)
{
	switch(type)
	{
	case L_CITY:
		{
			City* city = new City;
			city->target = city_target;
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
Location* World::CreateLocation(LOCATION type, const Vec2& pos, float range, int target, UnitGroup* group, bool allow_exact, int dungeon_levels)
{
	assert(group);
	assert(type != L_CITY); // not implemented - many methods currently assume that cities are at start of locations vector

	Vec2 pt = pos;
	if(range < 0.f)
	{
		pt = Vec2::Random(world_bounds.x, world_bounds.y);
		range = -range;
	}
	if(!FindPlaceForLocation(pt, range, allow_exact))
		return nullptr;

	int levels = -1;
	if(type == L_DUNGEON)
	{
		assert(target != -1);
		BaseLocation& base = g_base_locations[target];
		if(dungeon_levels == -1)
			levels = base.levels.Random();
		else if(dungeon_levels == 0)
			levels = base.levels.x;
		else if(dungeon_levels == 9)
			levels = base.levels.y;
		else
			levels = dungeon_levels;
	}

	Location* loc = CreateLocation(type, levels);
	loc->pos = pt;
	loc->type = type;
	loc->target = target;

	if(type == L_DUNGEON)
	{
		InsideLocation* inside = static_cast<InsideLocation*>(loc);
		inside->group = group;
		if(inside->group == UnitGroup::random)
			inside->group = g_base_locations[target].GetRandomGroup();
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
		if(group != UnitGroup::random)
			loc->group = group;
		else
		{
			switch(type)
			{
			case L_CITY:
				loc->group = UnitGroup::empty;
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
	}

	SetLocationImageAndName(loc);
	AddLocation(loc);
	return loc;
}

//=================================================================================================
void World::AddLocations(uint count, AddLocationsCallback clbk, float valid_dist)
{
	for(uint i = 0; i < count; ++i)
	{
		for(uint j = 0; j < 100; ++j)
		{
			Vec2 pt = Vec2::Random(world_border, float(world_size) - world_border);
			bool ok = true;

			// disallow when near other location
			for(Location* l : locations)
			{
				float dist = Vec2::Distance(pt, l->pos);
				if(dist < valid_dist)
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

	if(empty_locations)
	{
		--empty_locations;
		int index = locations.size() - 1;
		for(vector<Location*>::reverse_iterator rit = locations.rbegin(), rend = locations.rend(); rit != rend; ++rit, --index)
		{
			if(!*rit)
			{
				*rit = loc;
				loc->index = index;
				if(Net::IsOnline() && !net->prepare_world)
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
		if(Net::IsOnline() && !net->prepare_world)
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
	assert(VerifyLocation(index));
	delete locations[index];
	if(index == locations.size() - 1)
		locations.pop_back();
	else
	{
		locations[index] = nullptr;
		++empty_locations;
	}
}

//=================================================================================================
void World::GenerateWorld()
{
	world_size = def_world_size;
	world_bounds = Vec2(world_border, world_size - world_border);

	// create capital
	Vec2 pos = Vec2::Random(float(world_size) * 0.4f, float(world_size) * 0.6f);
	CreateCity(pos, CAPITAL);

	// create more cities
	for(uint i = 0, count = Random(3u, 4u); i < count; ++i)
	{
		for(int tries = 0; tries < 20; ++tries)
		{
			Vec2 parent_pos = locations[Rand() % locations.size()]->pos;
			float rot = Random(MAX_ANGLE);
			float dist = Random(300.f, 400.f);
			Vec2 pos = parent_pos + Vec2(cos(rot)*dist, sin(rot)*dist);
			if(pos.x < world_bounds.x || pos.y < world_bounds.x || pos.x > world_bounds.y || pos.y > world_bounds.y)
				continue;
			bool ok = true;
			for(Location* loc : locations)
			{
				float dist = Vec2::DistanceSquared(loc->pos, pos);
				if(dist < 250.f * 250.f)
				{
					ok = false;
					break;
				}
			}
			if(ok)
			{
				CreateCity(pos, CITY);
				break;
			}
		}
	}

	// player starts in one of cities
	uint cities = locations.size();
	uint start_location = Rand() % cities;

	// create villages
	for(uint i = 0, count = Random(10u, 15u); i < count; ++i)
	{
		for(int tries = 0; tries < 20; ++tries)
		{
			Vec2 parent_pos = locations[Rand() % cities]->pos;
			float rot = Random(MAX_ANGLE);
			float dist = Random(100.f, 250.f);
			Vec2 pos = parent_pos + Vec2(cos(rot)*dist, sin(rot)*dist);
			if(pos.x < world_bounds.x || pos.y < world_bounds.x || pos.x > world_bounds.y || pos.y > world_bounds.y)
				continue;
			bool ok = true;
			for(Location* loc : locations)
			{
				float dist = Vec2::DistanceSquared(loc->pos, pos);
				if(dist < 100.f * 100.f)
				{
					ok = false;
					break;
				}
			}
			if(ok)
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
			Vec2 pos = Vec2::Random(world_bounds.x, world_bounds.y);
			bool ok = true;
			for(Location* loc : locations)
			{
				float dist = Vec2::DistanceSquared(loc->pos, pos);
				if(dist < 400.f * 400.f)
				{
					ok = false;
					break;
				}
			}
			if(ok)
			{
				CreateCity(pos, VILLAGE);
				break;
			}
		}
	}

	// mark cities and villages as known
	settlements = locations.size();
	for(Location* loc : locations)
		loc->state = LS_KNOWN;

	// generate guaranteed locations, one of each type & target
	static const LOCATION guaranteed[] = {
		L_OUTSIDE, L_OUTSIDE,
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
	{
		Location* loc = new OutsideLocation;
		loc->index = locations.size();
		loc->pos = Vec2(-1000, -1000);
		loc->state = LS_HIDDEN;
		loc->type = L_ENCOUNTER;
		SetLocationImageAndName(loc);
		encounter_loc = loc->index;
		locations.push_back(loc);
	}

	CalculateTiles();

	// generate locations content
	bool guaranteed_moonwell = true;
	int index = 0, guaranteed_dungeon = 0;
	UnitGroup* forest_group = UnitGroup::Get("forest");
	UnitGroup* cave_group = UnitGroup::Get("cave");
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
				city.citizens_world = 0;
				city.st = 1;
				city.group = UnitGroup::empty;
				LocalVector2<Building*> buildings;
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
				if(guaranteed_dungeon == -1)
				{
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
				}
				else
				{
					switch(guaranteed_dungeon)
					{
					default:
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
					}

					++guaranteed_dungeon;
					if(target == 10)
						guaranteed_dungeon = -1;
				}

				BaseLocation& base = g_base_locations[target];
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
			if(guaranteed_moonwell)
			{
				guaranteed_moonwell = false;
				loc.target = MOONWELL;
				loc.st = 10;
				loc.group = forest_group;
				GetTileSt(loc.pos) = 10;
			}
			else
			{
				int& st = GetTileSt(loc.pos);
				if(st < 2)
					st = 2;
				else if(st > 10)
					st = 10;
				loc.st = st;
				loc.group = forest_group;
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
				loc.group = cave_group;
			}
			break;
		}

		SetLocationImageAndName(*it);
	}

	SmoothTiles();

	this->start_location = locations[start_location];
	tomir_spawned = false;
}

//=================================================================================================
void World::StartInLocation()
{
	state = State::ON_MAP;
	current_location_index = start_location->index;
	current_location = start_location;
	current_location->state = LS_ENTERED;
	world_pos = current_location->pos;
	game_level->location_index = current_location_index;
	game_level->location = current_location;

	// reveal near locations
	const Vec2& start_pos = start_location->pos;
	for(vector<Location*>::iterator it = locations.begin(), end = locations.end(); it != end; ++it)
	{
		if(!*it)
			continue;

		Location& loc = **it;
		if(loc.state == LS_UNKNOWN && Vec2::Distance(start_pos, loc.pos) <= 150.f)
			loc.state = LS_KNOWN;
	}
}

//=================================================================================================
void World::CalculateTiles()
{
	int ts = world_size / TILE_SIZE;
	tiles.resize(ts*ts);

	// set from near locations
	for(int y = 0; y < ts; ++y)
	{
		for(int x = 0; x < ts; ++x)
		{
			int index = x + y * ts;
			int st = Random(5, 15);
			Vec2 pos(float(x * TILE_SIZE) + 0.5f * TILE_SIZE, float(y * TILE_SIZE) + 0.5f * TILE_SIZE);
			for(uint i = 0; i < locations.size(); ++i)
			{
				Location* loc = locations[i];
				if(!Any(loc->type, L_CITY, L_DUNGEON))
					continue;
				float dist = Vec2::Distance(pos, loc->pos);
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
	int ts = world_size / TILE_SIZE;

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
					st += tiles[x - 1 + (y - 1)*ts];
				}
				if(y < ts - 1)
				{
					++count;
					st += tiles[x - 1 + (y + 1)*ts];
				}
			}
			if(x < ts - 1)
			{
				++count;
				st += tiles[x + 1 + y * ts];
				if(y > 0)
				{
					++count;
					st += tiles[x + 1 + (y - 1)*ts];
				}
				if(y < ts - 1)
				{
					++count;
					st += tiles[x + 1 + (y + 1)*ts];
				}
			}
			if(y > 0)
			{
				++count;
				st += tiles[x + (y - 1)*ts];
			}
			if(y < ts - 1)
			{
				++count;
				st += tiles[x + (y + 1)*ts];
			}
			tiles[x + y * ts] = st / count;
		}
	}

	// clamp values
	for(int i = 0; i < ts*ts; ++i)
		tiles[i] = Clamp(tiles[i], 2, 16);
}

//=================================================================================================
void World::CreateCity(const Vec2& pos, int target)
{
	Location* l = CreateLocation(L_CITY, -1, target);
	l->type = L_CITY;
	l->pos = pos;
	l->state = LS_UNKNOWN;
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
		case MOONWELL:
			l->image = LI_MOONWELL;
			l->name = txMoonwell;
			return;
		case ACADEMY:
			l->image = LI_ACADEMY;
			l->name = txAcademy;
			return;
		default:
			l->image = LI_FOREST;
			l->name = txForest;
			break;
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
	f << year;
	f << month;
	f << day;
	f << worldtime;
	f << day_timer;
	f << world_size;
	f << current_location_index;

	f << locations.size();
	Location* current;
	if(state == State::INSIDE_LOCATION || state == State::INSIDE_ENCOUNTER)
		current = current_location;
	else
		current = nullptr;
	byte check_id = 0;
	for(Location* loc : locations)
	{
		if(!loc)
			f << L_NULL;
		else
		{
			f << loc->type;
			if(loc->type == L_DUNGEON)
				f << loc->GetLastLevel() + 1;
			loc->Save(f, current == loc);
		}
		f << check_id;
		++check_id;
	}

	f << empty_locations;
	f << create_camp;
	f << world_pos;
	f << reveal_timer;
	f << encounter_chance;
	f << settlements;
	f << encounter_loc;
	f << travel_dir;
	if(state == State::INSIDE_ENCOUNTER)
	{
		f << travel_location_index;
		f << travel_start_pos;
		f << travel_target_pos;
		f << travel_timer;
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
			if(enc->pooled_string)
				f << *enc->pooled_string;
			else
				f.Write0();
			f << enc->st;
			f << enc->dont_attack;
			f << enc->timed;
		}
	}
	f << tiles;

	f << news.size();
	for(News* n : news)
	{
		f << n->add_time;
		f.WriteString2(n->text);
	}

	f << boss_levels;
	f << tomir_spawned;
}

//=================================================================================================
void World::Load(GameReader& f, LoadingHandler& loading)
{
	f >> state;
	f >> year;
	f >> month;
	f >> day;
	f >> worldtime;
	f >> day_timer;
	f >> world_size;
	world_bounds = Vec2(world_border, world_size - 16.f);
	f >> current_location_index;
	LoadLocations(f, loading);
	LoadNews(f);
	if(LOAD_VERSION < V_0_12)
		f.Skip<bool>(); // old first_city
	f >> boss_levels;
	f >> tomir_spawned;
}

//=================================================================================================
void World::LoadLocations(GameReader& f, LoadingHandler& loading)
{
	byte read_id,
		check_id = 0;

	uint count = f.Read<uint>();
	locations.resize(count);
	int index = -1;
	int current_index;
	if(state == State::INSIDE_LOCATION || state == State::INSIDE_ENCOUNTER)
		current_index = current_location_index;
	else
		current_index = -1;
	int step = 0;
	Location* academy = nullptr;
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
				case L_ENCOUNTER:
					loc = new OutsideLocation;
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
				default:
					assert(0);
					loc = new OutsideLocation;
					break;
				}

				loc->type = type;
				loc->index = index;
				loc->Load(f, current_index == index);
			}
			else
				loc = nullptr;
		}
		else
		{
			old::LOCATION_TOKEN loc_token;
			f >> loc_token;

			if(loc_token != old::LT_NULL)
			{
				switch(loc_token)
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

				loc->index = index;
				loc->Load(f, current_index == index);

				// remove old academy
				if(LOAD_VERSION < V_0_8 && loc->type == L_NULL)
					academy = loc;
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

		f >> read_id;
		if(read_id != check_id)
			throw Format("Error while reading location %s (%d).", loc ? loc->name.c_str() : "nullptr", index);
		++check_id;
	}
	f >> empty_locations;
	f >> create_camp;
	if(LOAD_VERSION < V_0_8)
		create_camp = 10 - create_camp;
	else if(LOAD_VERSION < V_0_11 && create_camp > 10)
		create_camp = 10;
	f >> world_pos;
	f >> reveal_timer;
	f >> encounter_chance;
	f >> settlements;
	f >> encounter_loc;
	f >> travel_dir;
	if(state == State::INSIDE_ENCOUNTER)
	{
		f >> travel_location_index;
		f >> travel_start_pos;
		if(LOAD_VERSION >= V_0_8)
			f >> travel_target_pos;
		else
			travel_target_pos = locations[travel_location_index]->pos;
		f >> travel_timer;
	}
	encounters.resize(f.Read<uint>(), nullptr);
	if(LOAD_VERSION >= V_0_9)
	{
		int index = 0;
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
				int quest_id;
				f >> quest_id;
				const string& dialog_id = f.ReadString1();
				if(!dialog_id.empty())
				{
					string* str = StringPool.Get();
					*str = dialog_id;
					quest_mgr->AddQuestRequest(quest_id, &enc->quest, [enc, str]
					{
						enc->dialog = static_cast<Quest_Scripted*>(enc->quest)->GetDialog(*str);
						StringPool.Free(str);
					});
				}
				else
					quest_mgr->AddQuestRequest(quest_id, &enc->quest);
				f >> enc->group;
				const string& text = f.ReadString1();
				if(text.empty())
				{
					enc->text = nullptr;
					enc->pooled_string = nullptr;
				}
				else
				{
					enc->pooled_string = StringPool.Get();
					*enc->pooled_string = text;
					enc->text = enc->pooled_string->c_str();
				}
				f >> enc->st;
				f >> enc->dont_attack;
				f >> enc->timed;
			}
			++index;
		}
	}
	if(LOAD_VERSION >= V_0_8)
		f >> tiles;
	else
		CalculateTiles();

	if(current_location_index != -1)
	{
		current_location = locations[current_location_index];
		if(current_location == academy)
		{
			current_location_index = GetNearestSettlement(academy->pos);
			locations[academy->index] = nullptr;
			++empty_locations;
			delete academy;
			current_location = locations[current_location_index];
			if(current_location->state == LS_KNOWN)
				current_location->state = LS_VISITED;
			world_pos = current_location->pos;
		}
	}
	else
		current_location = nullptr;
	game_level->location_index = current_location_index;
	game_level->location = current_location;
	if(game_level->location && Any(state, State::INSIDE_LOCATION, State::INSIDE_ENCOUNTER))
		game_level->Apply();
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
		f >> n->add_time;
		f.ReadString2(n->text);
	}
}

//=================================================================================================
// pre V_0_8
void World::LoadOld(GameReader& f, LoadingHandler& loading, int part, bool inside)
{
	if(part == 0)
	{
		f >> year;
		f >> month;
		f >> day;
		f >> worldtime;
		world_size = old::def_world_size;
		world_bounds = Vec2(world_border, world_size - 16.f);
		day_timer = 0.f;
		tomir_spawned = false;
	}
	else if(part == 1)
	{
		enum WORLDMAP_STATE
		{
			WS_MAIN,
			WS_TRAVEL,
			WS_ENCOUNTER
		} old_state;
		f >> old_state;
		switch(old_state)
		{
		case WS_MAIN:
			if(inside)
				state = State::INSIDE_LOCATION;
			else
				state = State::ON_MAP;
			break;
		case WS_TRAVEL:
			state = State::INSIDE_ENCOUNTER;
			break;
		case WS_ENCOUNTER:
			state = State::ENCOUNTER;
			break;
		}
		f >> current_location_index;
		LoadLocations(f, loading);
		if(state == State::INSIDE_ENCOUNTER)
		{
			// random encounter - should guards give team reward?
			bool guards_enc_reward;
			f >> guards_enc_reward;
			if(guards_enc_reward)
				script_mgr->GetVar("guards_enc_reward") = true;
		}
		else if(state == State::ENCOUNTER)
		{
			// bugfix
			state = State::TRAVEL;
			travel_start_pos = world_pos = locations[0]->pos;
			travel_location_index = 0;
			travel_timer = 1.f;
		}
	}
	else if(part == 2)
		LoadNews(f);
	else if(part == 3)
	{
		f.Skip<bool>(); // old first_city
		f >> boss_levels;
		locations[encounter_loc]->state = LS_HIDDEN;
	}
}

//=================================================================================================
void World::Write(BitStreamWriter& f)
{
	f << world_size;
	WriteTime(f);

	// locations
	f.WriteCasted<byte>(locations.size());
	for(Location* loc_ptr : locations)
	{
		if(!loc_ptr)
		{
			f.WriteCasted<byte>(L_NULL);
			continue;
		}

		Location& loc = *loc_ptr;
		f.WriteCasted<byte>(loc.type);
		if(loc.type == L_DUNGEON)
			f.WriteCasted<byte>(loc.GetLastLevel() + 1);
		else if(loc.type == L_CITY)
		{
			City& city = (City&)loc;
			f.WriteCasted<byte>(city.citizens);
			f.WriteCasted<word>(city.citizens_world);
		}
		f.WriteCasted<byte>(loc.state);
		f.WriteCasted<byte>(loc.target);
		f << loc.pos;
		f << loc.name;
		f.WriteCasted<byte>(loc.image);
	}
	f.WriteCasted<byte>(current_location_index);
	f << world_pos;

	// position on world map when inside encounter location
	if(state == State::INSIDE_ENCOUNTER)
	{
		f << true;
		f << travel_location_index;
		f << travel_start_pos;
		f << travel_target_pos;
		f << travel_timer;
	}
	else
		f << false;
}

//=================================================================================================
void World::WriteTime(BitStreamWriter& f)
{
	f << worldtime;
	f.WriteCasted<byte>(day);
	f.WriteCasted<byte>(month);
	f << year;
}

//=================================================================================================
bool World::Read(BitStreamReader& f)
{
	f >> world_size;
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
				word world_citizens;
				f >> citizens;
				f >> world_citizens;
				if(!f)
				{
					Error("Read world: Broken packet for city location %u.", index);
					return false;
				}

				City* city = new City;
				loc = city;
				city->citizens = citizens;
				city->citizens_world = world_citizens;
			}
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
	f.ReadCasted<byte>(current_location_index);
	f >> world_pos;
	if(!f)
	{
		Error("Read world: Broken packet for current location.");
		return false;
	}
	if(current_location_index == 255)
	{
		current_location_index = -1;
		current_location = nullptr;
		game_level->location_index = -1;
		game_level->location = nullptr;
	}
	else
	{
		if(current_location_index >= (int)locations.size() || !locations[current_location_index])
		{
			Error("Read world: Invalid location %d.", current_location_index);
			return false;
		}
		current_location = locations[current_location_index];
		game_level->location_index = current_location_index;
		game_level->location = current_location;
	}

	// position on world map when inside encounter locations
	bool inside_encounter;
	f >> inside_encounter;
	if(!f)
	{
		Error("Read world: Broken packet for in travel data.");
		return false;
	}
	if(inside_encounter)
	{
		state = State::INSIDE_ENCOUNTER;
		f >> travel_location_index;
		f >> travel_start_pos;
		f >> travel_target_pos;
		f >> travel_timer;
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
	f.ReadCasted<byte>(day);
	f.ReadCasted<byte>(month);
	f >> year;
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
	return Format(txDate, year, month + 1, day + 1);
}

//=================================================================================================
cstring World::GetDate(int year, int month, int day) const
{
	return Format(txDate, year, month + 1, day + 1);
}

//=================================================================================================
int World::GetRandomSettlementIndex(int excluded) const
{
	int index = Rand() % settlements;
	if(index == excluded)
		index = (index + 1) % settlements;
	return index;
}

//=================================================================================================
int World::GetRandomFreeSettlementIndex(int excluded) const
{
	int index = Rand() % settlements,
		tries = (int)settlements;
	while((index == excluded || locations[index]->active_quest) && tries > 0)
	{
		index = (index + 1) % settlements;
		--tries;
	}
	return (tries == 0 ? -1 : index);
}

//=================================================================================================
int World::GetRandomCityIndex(int excluded) const
{
	int count = 0;
	while(LocationHelper::IsCity(locations[count]))
		++count;

	int index = Rand() % count;
	if(index == excluded)
	{
		++index;
		if(index == count)
			index = 0;
	}

	return index;
}

//=================================================================================================
// Get random settlement excluded from used list
int World::GetRandomSettlementIndex(const vector<int>& used, int target) const
{
	int index = Rand() % settlements;

	while(true)
	{
		if(target != ANY_TARGET && locations[index]->target != target)
		{
			index = (index + 1) % settlements;
			continue;
		}

		bool ok = true;
		for(vector<int>::const_iterator it = used.begin(), end = used.end(); it != end; ++it)
		{
			if(*it == index)
			{
				index = (index + 1) % settlements;
				ok = false;
				break;
			}
		}

		if(ok)
			break;
	}

	return index;
}

//=================================================================================================
int World::GetClosestLocation(LOCATION type, const Vec2& pos, int target, int flags)
{
	int best = -1, index = 0;
	float dist, best_dist;
	const bool allow_active = IsSet(flags, F_ALLOW_ACTIVE);
	const bool excluded = IsSet(flags, F_EXCLUDED);

	for(vector<Location*>::iterator it = locations.begin(), end = locations.end(); it != end; ++it, ++index)
	{
		Location* loc = *it;
		if(!loc || loc->type != type)
			continue;
		if(!allow_active && loc->active_quest)
			continue;
		if(target != ANY_TARGET)
		{
			bool ok = (loc->target == target);
			if(ok == excluded)
				continue;
		}
		dist = Vec2::Distance(loc->pos, pos);
		if(best == -1 || dist < best_dist)
		{
			best = index;
			best_dist = dist;
		}
	}

	return best;
}

//=================================================================================================
int World::GetClosestLocation(LOCATION type, const Vec2& pos, const int* targets, int n_targets, int flags)
{
	int best = -1, index = 0;
	float dist, best_dist;
	const bool allow_active = IsSet(flags, F_ALLOW_ACTIVE);
	const bool excluded = IsSet(flags, F_EXCLUDED);

	for(vector<Location*>::iterator it = locations.begin(), end = locations.end(); it != end; ++it, ++index)
	{
		Location* loc = *it;
		if(!loc || loc->type != type)
			continue;
		if(!allow_active && loc->active_quest)
			continue;
		bool ok = false;
		for(int i = 0; i < n_targets; ++i)
		{
			if(loc->target == i)
			{
				ok = true;
				break;
			}
		}
		if(ok == excluded)
			continue;
		dist = Vec2::Distance(loc->pos, pos);
		if(best == -1 || dist < best_dist)
		{
			best = index;
			best_dist = dist;
		}
	}

	return best;
}

//=================================================================================================
bool World::FindPlaceForLocation(Vec2& pos, float range, bool allow_exact)
{
	Vec2 pt;

	if(allow_exact)
		pt = pos;
	else
		pt = (pos + Vec2::RandomCirclePt(range)).Clamped(Vec2(world_bounds.x, world_bounds.x), Vec2(world_bounds.y, world_bounds.y));

	for(int i = 0; i < 20; ++i)
	{
		bool valid = true;
		for(vector<Location*>::iterator it = locations.begin(), end = locations.end(); it != end; ++it)
		{
			if(*it && Vec2::Distance(pt, (*it)->pos) < 32)
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
			pt = (pos + Vec2::RandomCirclePt(range)).Clamped(Vec2(world_bounds.x, world_bounds.x), Vec2(world_bounds.y, world_bounds.y));
	}

	return false;
}

//=================================================================================================
// Searches for location with selected spawn group
// If cleared respawns enemies
// If nothing found creates camp
int World::GetRandomSpawnLocation(const Vec2& pos, UnitGroup* group, float range)
{
	int best_ok = -1, best_empty = -1, index = settlements;
	float ok_range, empty_range, dist;

	for(vector<Location*>::iterator it = locations.begin() + settlements, end = locations.end(); it != end; ++it, ++index)
	{
		if(!*it)
			continue;
		if(!(*it)->active_quest && (*it)->type == L_DUNGEON)
		{
			InsideLocation* inside = (InsideLocation*)*it;
			if((*it)->state == LS_CLEARED || inside->group == group || !inside->group)
			{
				dist = Vec2::Distance(pos, (*it)->pos);
				if(dist <= range)
				{
					if(inside->group == group)
					{
						if(best_ok == -1 || dist < ok_range)
						{
							best_ok = index;
							ok_range = dist;
						}
					}
					else
					{
						if(best_empty == -1 || dist < empty_range)
						{
							best_empty = index;
							empty_range = dist;
						}
					}
				}
			}
		}
	}

	if(best_ok != -1)
	{
		locations[best_ok]->reset = true;
		return best_ok;
	}

	if(best_empty != -1)
	{
		locations[best_empty]->group = group;
		locations[best_empty]->reset = true;
		return best_empty;
	}

	return CreateCamp(pos, group, range / 2);
}

//=================================================================================================
City* World::GetRandomSettlement(delegate<bool(City*)> pred)
{
	int start_index = Rand() % settlements;
	int index = start_index;
	do
	{
		City* loc = static_cast<City*>(locations[index]);
		if(pred(loc))
			return loc;
		index = (index + 1) % settlements;
	}
	while(index != start_index);
	return nullptr;
}

//=================================================================================================
Location* World::GetRandomSettlement(Location* loc)
{
	return GetRandomSettlement(loc->index);
}

//=================================================================================================
Location* World::GetRandomSettlementWeighted(delegate<float(Location*)> func)
{
	float best_value = -1.f;
	int best_index = -1;
	int start_index = Rand() % settlements;
	int index = start_index;
	do
	{
		Location* loc = locations[index];
		float result = func(loc);
		if(result >= 0.f && result > best_value)
		{
			best_value = result;
			best_index = index;
		}
		index = (index + 1) % settlements;
	}
	while(index != start_index);
	return (best_index != -1 ? locations[best_index] : nullptr);
}

//=================================================================================================
void World::ExitToMap()
{
	if(state == State::INSIDE_ENCOUNTER)
	{
		state = State::TRAVEL;
		current_location_index = -1;
		current_location = nullptr;
		game_level->location_index = -1;
		game_level->location = nullptr;
	}
	else
		state = State::ON_MAP;
}

//=================================================================================================
void World::ChangeLevel(int index, bool encounter)
{
	state = encounter ? State::INSIDE_ENCOUNTER : State::INSIDE_LOCATION;
	current_location_index = index;
	current_location = locations[index];
	game_level->location_index = current_location_index;
	game_level->location = current_location;
}

//=================================================================================================
void World::StartInLocation(Location* loc)
{
	locations.push_back(loc);
	current_location_index = locations.size() - 1;
	current_location = loc;
	state = State::INSIDE_LOCATION;
	loc->index = current_location_index;
	game_level->location_index = current_location_index;
	game_level->location = loc;
	game_level->is_open = true;
}

//=================================================================================================
void World::StartEncounter()
{
	state = State::INSIDE_ENCOUNTER;
	current_location_index = encounter_loc;
	current_location = locations[current_location_index];
	game_level->location_index = current_location_index;
	game_level->location = current_location;
}

//=================================================================================================
void World::Travel(int index, bool order)
{
	if(state == State::TRAVEL)
		return;

	// leave current location
	if(game_level->is_open)
	{
		game->LeaveLocation();
		game_level->is_open = false;
	}

	state = State::TRAVEL;
	current_location = nullptr;
	current_location_index = -1;
	game_level->location = nullptr;
	game_level->location_index = -1;
	travel_timer = 0.f;
	travel_start_pos = world_pos;
	travel_location_index = index;
	travel_target_pos = locations[travel_location_index]->pos;
	travel_dir = AngleLH(world_pos.x, world_pos.y, travel_target_pos.x, travel_target_pos.y);
	reveal_timer = 0.f;
	if(Net::IsLocal())
		travel_first_frame = true;

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
	if(game_level->is_open)
	{
		game->LeaveLocation();
		game_level->is_open = false;
	}

	state = State::TRAVEL;
	current_location = nullptr;
	current_location_index = -1;
	game_level->location = nullptr;
	game_level->location_index = -1;
	travel_timer = 0.f;
	travel_start_pos = world_pos;
	travel_location_index = -1;
	travel_target_pos = pos;
	travel_dir = AngleLH(world_pos.x, world_pos.y, travel_target_pos.x, travel_target_pos.y);
	reveal_timer = 0.f;
	if(Net::IsLocal())
		travel_first_frame = true;

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
	if(travel_first_frame)
	{
		travel_first_frame = false;
		return;
	}

	day_timer += dt;
	if(day_timer >= 1.f)
	{
		// another day passed
		--day_timer;
		if(Net::IsLocal())
			Update(1, UM_TRAVEL);
	}

	float dist = Vec2::Distance(travel_start_pos, travel_target_pos);
	travel_timer += dt;
	if(travel_timer * 3 >= dist / TRAVEL_SPEED)
	{
		// end of travel
		if(Net::IsLocal())
		{
			team->OnTravel(Vec2::Distance(world_pos, travel_target_pos));
			EndTravel();
		}
		else
			world_pos = travel_target_pos;
	}
	else
	{
		Vec2 dir = travel_target_pos - travel_start_pos;
		float travel_dist = travel_timer / dist * TRAVEL_SPEED * 3;
		world_pos = travel_start_pos + dir * travel_dist;
		if(Net::IsLocal())
			team->OnTravel(travel_dist);

		// reveal nearby locations, check encounters
		reveal_timer += dt;
		if(Net::IsLocal() && reveal_timer >= 0.25f)
		{
			reveal_timer = 0;

			UnitGroup* group = nullptr;
			for(Location* ploc : locations)
			{
				if(!ploc)
					continue;
				Location& loc = *ploc;
				float dist = Vec2::Distance(world_pos, loc.pos);
				if(dist <= 50.f)
				{
					if(loc.state != LS_CLEARED && dist <= 32.f && loc.group && loc.group->encounter_chance != 0)
					{
						group = loc.group;
						int chance = loc.group->encounter_chance;
						if(loc.type == L_CAMP)
							chance *= 2;
						encounter_chance += chance;
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
				if(Vec2::Distance(encounter->pos, world_pos) < encounter->range)
				{
					if(!encounter->check_func || encounter->check_func())
					{
						encounter_chance += encounter->chance;
						enc = index;
					}
				}
			}

			encounter_chance += 1;

			if(Rand() % 500 < ((int)encounter_chance) - 25 || (gui->HaveFocus() && GKey.DebugKey(Key::E)))
			{
				encounter_chance = 0.f;
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

	encounter.st = GetTileSt(world_pos);
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
		Quest_Crazies::State c_state = quest_mgr->quest_crazies->crazies_state;

		bool special = false;
		bool golem = (quest_mgr->quest_mages2->mages_state >= Quest_Mages2::State::Encounter
			&& quest_mgr->quest_mages2->mages_state < Quest_Mages2::State::Completed && Rand() % 3 == 0) || GKey.DebugKey(Key::G);
		bool crazy = (c_state == Quest_Crazies::State::TalkedWithCrazy && (Rand() % 2 == 0 || GKey.DebugKey(Key::S)));
		bool unk = (c_state >= Quest_Crazies::State::PickedStone && c_state < Quest_Crazies::State::End && (Rand() % 3 == 0 || GKey.DebugKey(Key::S)));
		if(quest_mgr->quest_mages2->mages_state == Quest_Mages2::State::Encounter && Rand() % 2 == 0)
			golem = true;
		if(c_state == Quest_Crazies::State::PickedStone && Rand() % 2 == 0)
			unk = true;

		if(!group)
		{
			if(Rand() % 6 == 0)
				group = UnitGroup::Get("bandits");
			else
				special = true;
		}
		else if(Rand() % 3 == 0)
			special = true;

		if(crazy || unk || golem || special || GKey.DebugKey(Key::Shift))
		{
			// special encounter
			encounter.mode = ENCOUNTER_SPECIAL;
			encounter.special = (SpecialEncounter)(Rand() % SE_MAX_NORMAL);
			if(unk)
				encounter.special = SE_UNK;
			else if(crazy)
				encounter.special = SE_CRAZY;
			else if(golem)
			{
				encounter.special = SE_GOLEM;
				quest_mgr->quest_mages2->paid = false;
			}
			else if(IsDebug() && input->Focus())
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
				if(!tomir_spawned && Rand() % 5 == 0)
				{
					encounter.special = SE_TOMIR;
					tomir_spawned = true;
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
			case SE_GOLEM:
				text = txEncGolem;
				break;
			case SE_CRAZY:
				text = txEncCrazy;
				break;
			case SE_UNK:
				text = txEncUnk;
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
			text = group->encounter_text.c_str();
			if(group->is_list)
			{
				for(UnitGroup::Entry& entry : group->entries)
				{
					if(entry.is_leader)
					{
						encounter.group = entry.group;
						break;
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
	if(travel_location_index != -1)
	{
		current_location_index = travel_location_index;
		current_location = locations[travel_location_index];
		travel_location_index = -1;
		game_level->location_index = current_location_index;
		game_level->location = current_location;
		Location& loc = *game_level->location;
		loc.SetVisited();
		world_pos = loc.pos;
	}
	else
		world_pos = travel_target_pos;
	if(Net::IsServer())
		Net::PushChange(NetChange::END_TRAVEL);
}

//=================================================================================================
void World::Warp(int index, bool order)
{
	if(game_level->is_open)
	{
		game->LeaveLocation(false, false);
		game_level->is_open = false;
	}

	current_location_index = index;
	current_location = locations[current_location_index];
	game_level->location_index = current_location_index;
	game_level->location = current_location;
	Location& loc = *current_location;
	loc.SetVisited();
	world_pos = loc.pos;

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
	if(game_level->is_open)
	{
		game->LeaveLocation(false, false);
		game_level->is_open = false;
	}

	world_pos = pos;
	current_location_index = -1;
	current_location = nullptr;
	game_level->location_index = -1;
	game_level->location = nullptr;

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
	encounters[index] = e;
	return e;
}

//=================================================================================================
// Set unit pos, dir according to travel_dir
void World::GetOutsideSpawnPoint(Vec3& pos, float& dir) const
{
	const float map_size = 256.f;
	const float dist = 40.f;

	// reverse dir, if you exit from north then you enter from south
	float entry_dir = Clip(travel_dir + PI);

	if(entry_dir < PI / 4 || entry_dir > 7.f / 4 * PI)
	{
		// east
		dir = PI / 2;
		if(entry_dir < PI / 4)
			pos = Vec3(map_size - dist, 0, Lerp(map_size / 2, dist, entry_dir / (PI / 4)));
		else
			pos = Vec3(map_size - dist, 0, Lerp(map_size - dist, map_size / 2, (entry_dir - 7.f / 4 * PI) / (PI / 4)));
	}
	else if(entry_dir < 3.f / 4 * PI)
	{
		// south
		dir = PI;
		pos = Vec3(Lerp(map_size - dist, dist, (entry_dir - 1.f / 4 * PI) / (PI / 2)), 0, dist);
	}
	else if(entry_dir < 5.f / 4 * PI)
	{
		// west
		dir = 3.f / 2 * PI;
		pos = Vec3(dist, 0, Lerp(dist, map_size - dist, (entry_dir - 3.f / 4 * PI) / (PI / 2)));
	}
	else
	{
		// north
		dir = 0;
		pos = Vec3(Lerp(dist, map_size - dist, (entry_dir - 5.f / 4 * PI) / (PI / 2)), 0, map_size - dist);
	}
}

//=================================================================================================
float World::GetTravelDays(float dist)
{
	return dist * World::MAP_KM_RATIO / World::TRAVEL_SPEED;
}

//=================================================================================================
// Set travel_dir using unit pos
// When crossed travel bariers simply calculate angle
// When using city gates find closest line segment point and then use above calculation on this point
void World::SetTravelDir(const Vec3& pos)
{
	const float map_size = 256.f;
	const float border = 33.f;
	Vec2 unit_pos = pos.XZ();

	if(Box2d(border, border, map_size - border, map_size - border).IsInside(unit_pos))
	{
		// not inside exit border, find closest line segment
		const float mini = 32.f;
		const float maxi = 256 - 32.f;
		float best_dist = 999.f, dist;
		Vec2 pt, close_pt;
		// check right
		dist = GetClosestPointOnLineSegment(Vec2(maxi, mini), Vec2(maxi, maxi), unit_pos, pt);
		if(dist < best_dist)
		{
			best_dist = dist;
			close_pt = pt;
		}
		// check left
		dist = GetClosestPointOnLineSegment(Vec2(mini, mini), Vec2(mini, maxi), unit_pos, pt);
		if(dist < best_dist)
		{
			best_dist = dist;
			close_pt = pt;
		}
		// check bottom
		dist = GetClosestPointOnLineSegment(Vec2(mini, mini), Vec2(maxi, mini), unit_pos, pt);
		if(dist < best_dist)
		{
			best_dist = dist;
			close_pt = pt;
		}
		// check top
		dist = GetClosestPointOnLineSegment(Vec2(mini, maxi), Vec2(maxi, maxi), unit_pos, pt);
		if(dist < best_dist)
			close_pt = pt;
		unit_pos = close_pt;
	}

	// point is outside of location borders
	if(unit_pos.x < border)
		travel_dir = Lerp(3.f / 4.f*PI, 5.f / 4.f*PI, 1.f - (unit_pos.y - border) / (map_size - border * 2));
	else if(unit_pos.x > map_size - border)
	{
		if(unit_pos.y > map_size / 2)
			travel_dir = Lerp(0.f, 1.f / 4 * PI, (unit_pos.y - map_size / 2) / (map_size - map_size / 2 - border));
		else
			travel_dir = Lerp(7.f / 4 * PI, PI * 2, (unit_pos.y - border) / (map_size - map_size / 2 - border));
	}
	else if(unit_pos.y < border)
		travel_dir = Lerp(5.f / 4 * PI, 7.f / 4 * PI, (unit_pos.x - border) / (map_size - border * 2));
	else
		travel_dir = Lerp(1.f / 4 * PI, 3.f / 4 * PI, 1.f - (unit_pos.x - border) / (map_size - border * 2));

	// convert RH angle to LH and then entry to exit dir
	travel_dir = Clip(ConvertAngle(travel_dir) + PI);
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
	n->add_time = worldtime;

	news.push_back(n);
}

//=================================================================================================
void World::AddBossLevel(const Int2& pos)
{
	if(Net::IsLocal())
		boss_levels.push_back(pos);
	else
		boss_level_mp = true;
}

//=================================================================================================
bool World::RemoveBossLevel(const Int2& pos)
{
	if(Net::IsLocal())
	{
		if(boss_level_mp)
		{
			boss_level_mp = false;
			return true;
		}
		return false;
	}
	return RemoveElementTry(boss_levels, pos);
}

//=================================================================================================
bool World::IsBossLevel(const Int2& pos) const
{
	if(Net::IsLocal())
		return boss_level_mp;
	for(const Int2& boss_pos : boss_levels)
	{
		if(boss_pos == pos)
			return true;
	}
	return false;
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

	DeleteElements(camp->chests);
	DeleteElements(camp->items);
	DeleteElements(camp->units);
	delete camp;

	if(remove)
		RemoveLocation(index);
}

//=================================================================================================
void World::AbadonLocation(Location* loc)
{
	assert(loc);

	// only works for OutsideLocation for now!
	assert(loc->outside && loc->type != L_CITY);

	OutsideLocation* outside = static_cast<OutsideLocation*>(loc);

	// if location is open
	if(loc == current_location)
	{
		// remove units
		for(Unit*& u : outside->units)
		{
			if(u->IsAlive() && game->pc->unit->IsEnemy(*u))
			{
				u->to_remove = true;
				game_level->to_remove.push_back(u);
			}
		}

		// remove items from chests
		for(Chest* chest : outside->chests)
		{
			if(!chest->GetUser())
				chest->items.clear();
		}
	}
	else
	{
		// delete units
		DeleteElements(outside->units, [](Unit* unit) { return unit->IsAlive() && game->pc->unit->IsEnemy(*unit); });

		// remove items from chests
		for(Chest* chest : outside->chests)
			chest->items.clear();
	}

	loc->group = UnitGroup::empty;
	loc->last_visit = worldtime;
}

//=================================================================================================
int World::FindWorldUnit(Unit* unit, int hint_loc, int hint_loc2, int* out_level)
{
	assert(unit);

	int level;

	if(hint_loc != -1)
	{
		if(GetLocation(hint_loc)->FindUnit(unit, &level))
		{
			if(out_level)
				*out_level = level;
			return hint_loc;
		}
	}

	if(hint_loc2 != -1 && hint_loc != hint_loc2)
	{
		if(GetLocation(hint_loc2)->FindUnit(unit, &level))
		{
			if(out_level)
				*out_level = level;
			return hint_loc2;
		}
	}

	for(uint i = 0; i < locations.size(); ++i)
	{
		Location* loc = locations[i];
		if(loc && i != hint_loc && i != hint_loc2 && loc->last_visit != -1 && loc->FindUnit(unit, &level))
		{
			if(out_level)
				*out_level = level;
			return i;
		}
	}

	return -1;
}

//=================================================================================================
void World::VerifyObjects()
{
	int errors = 0, e;

	for(Location* l : locations)
	{
		if(!l)
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
				for(InsideBuilding* ib : city->inside_buildings)
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
	assert(pos.x >= 0 || pos.y >= 0 || pos.x < (float)world_size || pos.y < (float)world_size);
	Int2 tile(int(pos.x / TILE_SIZE), int(pos.y / TILE_SIZE));
	int ts = world_size / TILE_SIZE;
	return tiles[tile.x + tile.y * ts];
}
