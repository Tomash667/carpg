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
#include "CaveLocation.h"
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
#include "Game.h"
#include "Debug.h"
#include "WorldMapGui.h"


//-----------------------------------------------------------------------------
const float World::TRAVEL_SPEED = 28.f;
const int start_year = 100;
const float world_size = 600.f;
const float world_border = 16.f;
const Vec2 world_bounds = Vec2(world_border, world_size - 16.f);
World W;


//-----------------------------------------------------------------------------
// z powodu zmian (po³¹czenie Location i Location2) musze tymczasowo u¿ywaæ tego w add_locations a potem w generate_world ustawiaæ odpowiedni obiekt
struct TmpLocation : public Location
{
	TmpLocation() : Location(false) {}

	void ApplyContext(LevelContext& ctx) {}
	void BuildRefidTable() {}
	bool FindUnit(Unit*, int*) { return false; }
	Unit* FindUnit(UnitData*, int&) { return nullptr; }
};


//-----------------------------------------------------------------------------
void World::InitOnce(WorldMapGui* gui)
{
	assert(gui); // FIXME - remove if working
	this->gui = gui;
	txDate = Str("dateFormat");
	txRandomEncounter = Str("randomEncounter");
	txEncCrazyMage = Str("encCrazyMage");
	txEncCrazyHeroes = Str("encCrazyHeroes");
	txEncCrazyCook = Str("encCrazyCook");
	txEncMerchant = Str("encMerchant");
	txEncHeroes = Str("encHeroes");
	txEncBanditsAttackTravelers = Str("encBanditsAttackTravelers");
	txEncHeroesAttack = Str("encHeroesAttack");
	txEncGolem = Str("encGolem");
	txEncCrazy = Str("encCrazy");
	txEncUnk = Str("encUnk");
	txEncBandits = Str("encBandits");
	txEncAnimals = Str("encAnimals");
	txEncOrcs = Str("encOrcs");
	txEncGoblins = Str("encGoblins");
}

void World::Cleanup()
{
	Reset();
}

void World::OnNewGame()
{
	travel_location_index = -1;
	empty_locations = 0;
	create_camp = 0;
	encounter_chance = 0.f;
	travel_dir = Random(MAX_ANGLE);
	year = start_year;
	day = 0;
	month = 0;
	worldtime = 0;
	first_city = true;
}

void World::Reset()
{
	DeleteElements(locations);
	DeleteElements(encounters);
	DeleteElements(news);
}

void World::Update(int days, UpdateMode mode)
{
	assert(days > 0);
	if(mode == UM_TRAVEL)
		assert(days == 1);

	UpdateDate(days);
	SpawnCamps(days);
	UpdateEncounters();
	UpdateLocations();
	UpdateNews();
	QM.Update(days);

	if(Net::IsLocal())
		Game::Get().UpdateQuests(days);

	if(mode == UM_TRAVEL)
		Team.Update(1, true);
	else if(mode == UM_NORMAL)
		Team.Update(days, false);

	// end of game
	if(year >= 160)
	{
		Info("Game over: you are too old.");
		Game& game = Game::Get();
		game.CloseAllPanels(true);
		game.koniec_gry = true;
		game.death_fade = 0.f;
		if(Net::IsOnline())
		{
			Net::PushChange(NetChange::GAME_STATS);
			Net::PushChange(NetChange::END_OF_GAME);
		}
	}
}

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

void World::SpawnCamps(int days)
{
	create_camp += days;
	if(create_camp >= 10)
	{
		create_camp = 0;
		SPAWN_GROUP group;
		switch(Rand() % 3)
		{
		case 0:
			group = SG_BANDITS;
			break;
		case 1:
			group = SG_ORCS;
			break;
		case 2:
			group = SG_GOBLINS;
			break;
		}
		CreateCamp(Vec2::Random(world_bounds.x, world_bounds.y), group, 128.f);
	}
}

void World::UpdateEncounters()
{
	for(vector<Encounter*>::iterator it = encounters.begin(), end = encounters.end(); it != end; ++it)
	{
		Encounter* e = *it;
		if(e && e->timed && e->quest->IsTimedout())
		{
			e->quest->OnTimeout(TIMEOUT_ENCOUNTER);
			e->quest->enc = -1;
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

void World::UpdateLocations()
{
	LoopAndRemove(locations, [this](Location* loc)
	{
		if(!loc || loc->active_quest || loc->type == L_ENCOUNTER)
			return false;
		if(loc->type == L_CAMP)
		{
			Camp* camp = (Camp*)loc;
			if(worldtime - camp->create_time >= 30
				&& current_location != loc // don't remove when team is inside
				&& (travel_location_index == -1 || locations[travel_location_index] != loc)) // don't remove when traveling to
			{
				// remove camp
				DeleteCamp(camp, false);
				return true;
			}
		}
		else if(loc->last_visit != -1 && worldtime - loc->last_visit >= 30)
		{
			loc->reset = true;
			if(loc->state == LS_CLEARED)
				loc->state = LS_ENTERED;
			if(loc->type == L_DUNGEON || loc->type == L_CRYPT)
			{
				InsideLocation* inside = (InsideLocation*)loc;
				if(inside->target != LABIRYNTH)
					inside->spawn = g_base_locations[inside->target].GetRandomSpawnGroup();
				if(inside->IsMultilevel())
					((MultiInsideLocation*)inside)->Reset();
			}
		}
		return false;
	});
}

void World::UpdateNews()
{
	LoopAndRemove(news, [this](News* news)
	{
		if(worldtime - news->add_time > 30)
		{
			delete news;
			return true;
		}
		return false;
	});
}

int World::CreateCamp(const Vec2& pos, SPAWN_GROUP group, float range, bool allow_exact)
{
	Camp* loc = new Camp;
	loc->state = LS_UNKNOWN;
	loc->active_quest = nullptr;
	loc->GenerateName();
	loc->type = L_CAMP;
	loc->image = LI_CAMP;
	loc->last_visit = -1;
	loc->st = Random(3, 15);
	loc->reset = false;
	loc->spawn = group;
	loc->pos = pos;
	loc->create_time = worldtime;

	FindPlaceForLocation(loc->pos, range, allow_exact);

	return AddLocation(loc);
}

Location* World::CreateLocation(LOCATION type, int levels, bool is_village)
{
	switch(type)
	{
	case L_CITY:
		{
			City* city = new City;
			if(is_village)
			{
				city->settlement_type = City::SettlementType::Village;
				city->image = LI_VILLAGE;
			}
			else
				city->image = LI_CITY;
			return city;
		}
	case L_CAVE:
		{
			CaveLocation* cave = new CaveLocation;
			cave->image = LI_CAVE;
			return cave;
		}
	case L_CAMP:
		{
			Camp* camp = new Camp;
			camp->image = LI_CAMP;
			return camp;
		}
	case L_FOREST:
	case L_ENCOUNTER:
	case L_MOONWELL:
	case L_ACADEMY:
		{
			LOCATION_IMAGE image;
			switch(type)
			{
			case L_FOREST:
			case L_ENCOUNTER:
				image = LI_FOREST;
				break;
			case L_MOONWELL:
				image = LI_MOONWELL;
				break;
			case L_ACADEMY:
				image = LI_ACADEMY;
				break;
			}
			OutsideLocation* loc = new OutsideLocation;
			loc->image = image;
			return loc;
		}
	case L_DUNGEON:
	case L_CRYPT:
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
			loc->image = (type == L_DUNGEON ? LI_DUNGEON : LI_CRYPT);
			return loc;
		}
	default:
		assert(0);
		return nullptr;
	}
}

// Create location
// If range<0 then position is random and range is positive
// Dungeon levels
//	-1 - random count
//	0 - minimum randomly generated
//	9 - maximum randomly generated
//	other - used number
Location* World::CreateLocation(LOCATION type, const Vec2& pos, float range, int target, SPAWN_GROUP spawn, bool allow_exact, int dungeon_levels)
{
	Vec2 pt = pos;
	if(range < 0.f)
	{
		pt = Vec2::Random(world_bounds.x, world_bounds.y);
		range = -range;
	}
	if(!FindPlaceForLocation(pt, range, allow_exact))
		return nullptr;

	int levels = -1;
	if(type == L_DUNGEON || type == L_CRYPT)
	{
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

	if(type == L_DUNGEON || type == L_CRYPT)
	{
		InsideLocation* inside = (InsideLocation*)loc;
		inside->target = target;
		if(target == LABIRYNTH)
		{
			if(spawn == SG_RANDOM)
				inside->spawn = SG_UNDEAD;
			else
				inside->spawn = spawn;
			inside->st = Random(8, 15);
		}
		else
		{
			if(spawn == SG_RANDOM)
				inside->spawn = g_base_locations[target].GetRandomSpawnGroup();
			else
				inside->spawn = spawn;
			inside->st = Random(3, 15);
		}
	}
	else
	{
		loc->st = Random(3, 13);
		if(spawn != SG_RANDOM)
			loc->spawn = spawn;
	}

	loc->GenerateName();
	AddLocation(loc);
	return loc;
}

void World::AddLocations(uint count, AddLocationsCallback clbk, float valid_dist, bool unique_name)
{
	for(uint i = 0; i < count; ++i)
	{
		for(uint j = 0; j < 100; ++j)
		{
			Vec2 pt = Vec2::Random(world_border, world_size - world_border);
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

			auto type = clbk(i);
			Location* l = CreateLocation(type.first, -1, type.second);
			l->type = type.first;
			l->pos = pt;
			l->state = LS_UNKNOWN;
			l->GenerateName();

			// check if name is unique
			if(unique_name && locations.size() > 1)
			{
				bool exists;
				do
				{
					exists = false;

					for(Location* l2 : locations)
					{
						if(l2->name == l->name)
						{
							exists = true;
							l->GenerateName();
							break;
						}
					}
				}
				while(exists);
			}

			l->index = locations.size();
			locations.push_back(l);
			break;
		}
	}
}

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
				if(Net::IsOnline())
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
		if(Net::IsOnline())
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

void World::AddLocationAtIndex(Location* loc)
{
	assert(loc && loc->index >= 0);
	if(loc->index >= (int)locations.size())
		locations.resize(loc->index + 1, nullptr);
	assert(!locations[loc->index]);
	locations[loc->index] = loc;
}

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

void World::GenerateWorld(int start_location_type, int start_location_target)
{
	// generate cities
	uint count = Random(3, 5);
	AddLocations(count, [](uint index) { return std::make_pair(L_CITY, false); }, 200.f, true);

	// player starts in one of cities
	uint start_location = Rand() % count;

	// generate villages
	count = 5 - count + Random(10, 15);
	AddLocations(count, [](uint index) { return std::make_pair(L_CITY, true); }, 100.f, true);

	// mark cities and villages as known
	settlements = locations.size();
	for(Location* loc : locations)
		loc->state = LS_KNOWN;

	// generate guaranteed locations, one of each type & target
	static const LOCATION guaranteed[] = {
		L_MOONWELL,
		L_FOREST,
		L_CAVE,
		L_CRYPT, L_CRYPT,
		L_DUNGEON, L_DUNGEON, L_DUNGEON, L_DUNGEON, L_DUNGEON, L_DUNGEON, L_DUNGEON, L_DUNGEON
	};
	AddLocations(countof(guaranteed), [](uint index) { return std::make_pair(guaranteed[index], false); }, 32.f, false);

	// generate other locations
	static const LOCATION locs[] = {
		L_CAVE,
		L_DUNGEON,
		L_DUNGEON,
		L_DUNGEON,
		L_CRYPT,
		L_FOREST
	};
	AddLocations(100 - locations.size(), [](uint index) { return std::make_pair(locs[Rand() % countof(locs)], false); }, 32.f, false);

	// create location for random encounter
	{
		Location* loc = new OutsideLocation;
		loc->index = locations.size();
		loc->pos = Vec2(-1000, -1000);
		loc->name = txRandomEncounter;
		loc->state = LS_VISITED;
		loc->image = LI_FOREST;
		loc->type = L_ENCOUNTER;
		encounter_loc = loc->index;
		locations.push_back(loc);
	}

	// reveal near locations, generate content
	int index = 0, guaranteed_dungeon = 0, guaranteed_crypt = 0;
	const Vec2& start_pos = locations[start_location]->pos;
	for(vector<Location*>::iterator it = locations.begin(), end = locations.end(); it != end; ++it, ++index)
	{
		if(!*it)
			continue;

		Location& loc = **it;
		if(loc.state == LS_UNKNOWN && Vec2::Distance(start_pos, loc.pos) <= 100.f)
			loc.state = LS_KNOWN;

		if(start_location_type == loc.type && start_location_target == -1)
		{
			start_location = index;
			start_location_type = -1;
		}

		if(loc.type == L_CITY)
		{
			City& city = (City&)loc;
			city.citizens = 0;
			city.citizens_world = 0;
			LocalVector2<Building*> buildings;
			city.GenerateCityBuildings(buildings.Get(), true);
			city.buildings.reserve(buildings.size());
			for(Building* b : buildings)
			{
				CityBuilding& cb = Add1(city.buildings);
				cb.type = b;
			}

			if(start_location_type == L_CITY && start_location_target == 1 && city.settlement_type == City::SettlementType::Village)
			{
				start_location = index;
				start_location_type = -1;
			}
		}
		else if(loc.type == L_DUNGEON || loc.type == L_CRYPT)
		{
			InsideLocation* inside;

			int target;
			if(loc.type == L_CRYPT)
			{
				if(guaranteed_crypt == -1)
					target = (Rand() % 2 == 0 ? HERO_CRYPT : MONSTER_CRYPT);
				else if(guaranteed_crypt == 0)
				{
					target = HERO_CRYPT;
					++guaranteed_crypt;
				}
				else
				{
					target = MONSTER_CRYPT;
					guaranteed_crypt = -1;
				}
			}
			else
			{
				if(guaranteed_dungeon == -1)
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
						target = LABIRYNTH;
						break;
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
						target = LABIRYNTH;
						break;
					}

					++guaranteed_dungeon;
					if(target == 8)
						guaranteed_dungeon = -1;
				}
			}

			BaseLocation& base = g_base_locations[target];
			int poziomy = base.levels.Random();

			if(poziomy == 1)
				inside = new SingleInsideLocation;
			else
				inside = new MultiInsideLocation(poziomy);

			inside->type = loc.type;
			inside->image = loc.image;
			inside->state = loc.state;
			inside->pos = loc.pos;
			inside->name = loc.name;
			delete &loc;
			*it = inside;

			inside->target = target;
			if(target == LABIRYNTH)
			{
				inside->st = Random(8, 15);
				inside->spawn = SG_UNDEAD;
			}
			else
			{
				inside->spawn = base.GetRandomSpawnGroup();
				if(Vec2::Distance(inside->pos, start_pos) < 100)
					inside->st = Random(3, 6);
				else
					inside->st = Random(3, 15);
			}

			if(start_location_type == loc.type && start_location_target == target)
			{
				start_location = index;
				start_location_type = -1;
			}
		}
		else if(loc.type == L_CAVE || loc.type == L_FOREST || loc.type == L_ENCOUNTER)
		{
			if(Vec2::Distance(loc.pos, start_pos) < 100)
				loc.st = Random(3, 6);
			else
				loc.st = Random(3, 13);
		}
		else if(loc.type == L_MOONWELL)
			loc.st = 10;
	}

	state = State::ON_MAP;
	current_location_index = start_location;
	current_location = locations[current_location_index];
	world_pos = current_location->pos;
	L.location_index = current_location_index;
	L.location = current_location;
}

void World::Save(GameWriter& f)
{
	f << state;
	f << year;
	f << month;
	f << day;
	f << worldtime;
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
		LOCATION_TOKEN loc_token;
		if(loc)
			loc_token = loc->GetToken();
		else
			loc_token = LT_NULL;
		f << loc_token;

		if(loc_token != LT_NULL)
		{
			if(loc_token == LT_MULTI_DUNGEON)
			{
				int levels = ((MultiInsideLocation*)loc)->levels.size();
				f << levels;
			}
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
		f << travel_day;
		f << travel_start_pos;
		f << travel_timer;
	}
	f << encounters.size();

	f << news.size();
	for(News* n : news)
	{
		f << n->add_time;
		f.WriteString2(n->text);
	}

	f << first_city;
	f << boss_levels;
}

void World::Load(GameReader& f, LoadingHandler& loading)
{
	f >> state;
	f >> year;
	f >> month;
	f >> day;
	f >> worldtime;
	f >> current_location_index;
	LoadLocations(f, loading);
	LoadNews(f);
	f >> first_city;
	f >> boss_levels;
}

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
	for(Location*& loc : locations)
	{
		++index;
		LOCATION_TOKEN loc_token;
		f >> loc_token;

		if(loc_token != LT_NULL)
		{
			switch(loc_token)
			{
			case LT_OUTSIDE:
				loc = new OutsideLocation;
				break;
			case LT_CITY:
			case LT_VILLAGE_OLD:
				loc = new City;
				break;
			case LT_CAVE:
				loc = new CaveLocation;
				break;
			case LT_SINGLE_DUNGEON:
				loc = new SingleInsideLocation;
				break;
			case LT_MULTI_DUNGEON:
				{
					int levels = f.Read<int>();
					loc = new MultiInsideLocation(levels);
				}
				break;
			case LT_CAMP:
				loc = new Camp;
				break;
			default:
				assert(0);
				loc = new OutsideLocation;
				break;
			}

			loc->index = index;
			loc->Load(f, current_index == index, loc_token);
		}
		else
			loc = nullptr;

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
	f >> world_pos;
	f >> reveal_timer;
	f >> encounter_chance;
	f >> settlements;
	f >> encounter_loc;
	f >> travel_dir;
	if(state == State::INSIDE_ENCOUNTER)
	{
		f >> travel_location_index;
		f >> travel_day;
		f >> travel_start_pos;
		f >> travel_timer;
	}
	encounters.resize(f.Read<uint>(), nullptr);

	L.location_index = current_location_index;
	if(current_location_index != -1)
	{
		current_location = locations[current_location_index];
		L.location = current_location;
	}
	else
	{
		current_location = nullptr;
		L.location = nullptr;
	}
}

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

void World::LoadOld(GameReader& f, LoadingHandler& loading, int part)
{
	if(part == 0)
	{
		f >> year;
		f >> month;
		f >> day;
		f >> worldtime;
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
			// random encounter - should guards give team reward
			bool guards_enc_reward;
			f >> guards_enc_reward;
			if(guards_enc_reward)
				SM.GetVar("guards_enc_reward") = true;
		}
		else if(state == State::ENCOUNTER)
		{
			// bugfix
			state = State::TRAVEL;
			travel_start_pos = world_pos = locations[0]->pos;
			travel_location_index = 0;
			travel_timer = 1.f;
			travel_day = 0;
		}
		if(LOAD_VERSION < V_0_3)
			travel_dir = Clip(-travel_dir);
		encounters.resize(f.Read<uint>(), nullptr);
	}
	else if(part == 2)
		LoadNews(f);
	else if(part == 3)
	{
		f >> first_city;
		f >> boss_levels;
	}
}

void World::Write(BitStreamWriter& f)
{
	// time
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
		if(loc.type == L_DUNGEON || loc.type == L_CRYPT)
			f.WriteCasted<byte>(loc.GetLastLevel() + 1);
		else if(loc.type == L_CITY)
		{
			City& city = (City&)loc;
			f.WriteCasted<byte>(city.citizens);
			f.WriteCasted<word>(city.citizens_world);
			f.WriteCasted<byte>(city.settlement_type);
		}
		f.WriteCasted<byte>(loc.state);
		f << loc.pos;
		f << loc.name;
		f.WriteCasted<byte>(loc.image);
	}
	f.WriteCasted<byte>(current_location_index);

	// position on world map when inside encounter location
	if(state == State::INSIDE_ENCOUNTER)
	{
		f << true;
		f << travel_location_index;
		f << travel_day;
		f << travel_timer;
		f << travel_start_pos;
		f << world_pos;
	}
	else
		f << false;
}

void World::WriteTime(BitStreamWriter& f)
{
	f << worldtime;
	f.WriteCasted<byte>(day);
	f.WriteCasted<byte>(month);
	f << year;
}

bool World::Read(BitStreamReader& f)
{
	// time
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
		case L_CRYPT:
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
			loc = new CaveLocation;
			break;
		case L_FOREST:
		case L_ENCOUNTER:
		case L_CAMP:
		case L_MOONWELL:
		case L_ACADEMY:
			loc = new OutsideLocation;
			break;
		case L_CITY:
			{
				byte citizens;
				word world_citizens;
				byte type;
				f >> citizens;
				f >> world_citizens;
				f >> type;
				if(!f)
				{
					Error("Read world: Broken packet for city location %u.", index);
					return false;
				}

				City* city = new City;
				loc = city;
				city->citizens = citizens;
				city->citizens_world = world_citizens;
				city->settlement_type = (City::SettlementType)type;
			}
			break;
		default:
			Error("Read world: Unknown location type %d for location %u.", type, index);
			return false;
		}

		// location data
		loc->index = index;
		f.ReadCasted<byte>(loc->state);
		f >> loc->pos;
		f >> loc->name;
		f.ReadCasted<byte>(loc->image);
		if(!f)
		{
			Error("Read world: Broken packet(2) for location %u.", index);
			return false;
		}
		loc->type = type;
		if(loc->state > LS_CLEARED)
		{
			Error("Read world: Invalid state %d for location %u.", loc->state, index);
			return false;
		}

		++index;
	}

	// current location
	f.ReadCasted<byte>(current_location_index);
	if(!f)
	{
		Error("Read world: Broken packet for current location.");
		return false;
	}
	if(current_location_index >= (int)locations.size() || !locations[current_location_index])
	{
		Error("Read world: Invalid location %d.", current_location_index);
		return false;
	}
	current_location = locations[current_location_index];
	world_pos = current_location->pos;
	L.location_index = current_location_index;
	L.location = current_location;
	L.location->state = LS_VISITED;

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
		f >> travel_day;
		f >> travel_timer;
		f >> travel_start_pos;
		f >> world_pos;
		if(!f)
		{
			Error("Read world: Broken packet for in travel data (2).");
			return false;
		}
	}

	return true;
}

void World::ReadTime(BitStreamReader& f)
{
	f >> worldtime;
	f.ReadCasted<byte>(day);
	f.ReadCasted<byte>(month);
	f >> year;
}

bool World::IsSameWeek(int worldtime2) const
{
	if(worldtime2 == -1)
		return false;
	int week = worldtime / 10;
	int week2 = worldtime2 / 10;
	return week == week2;
}

cstring World::GetDate() const
{
	return Format("%d-%d-%d", year, month + 1, day + 1);
}

int World::GetRandomSettlementIndex(int excluded) const
{
	int index = Rand() % excluded;
	if(index == excluded)
		index = (index + 1) % excluded;
	return index;
}

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

// Get random 0-settlement, 1-city, 2-village; excluded from used list
int World::GetRandomSettlementIndex(const vector<int>& used, int type) const
{
	int index = Rand() % settlements;

	while(true)
	{
		if(type != 0)
		{
			City* city = (City*)locations[index];
			if(type == 1)
			{
				if(city->settlement_type == City::SettlementType::Village)
				{
					index = (index + 1) % settlements;
					continue;
				}
			}
			else
			{
				if(city->settlement_type == City::SettlementType::City)
				{
					index = (index + 1) % settlements;
					continue;
				}
			}
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

int World::GetClosestLocation(LOCATION type, const Vec2& pos, int target)
{
	int best = -1, index = 0;
	float dist, best_dist;

	for(vector<Location*>::iterator it = locations.begin(), end = locations.end(); it != end; ++it, ++index)
	{
		Location* loc = *it;
		if(!loc || loc->active_quest || loc->type != type)
			continue;
		if(target != -1 && ((InsideLocation*)loc)->target != target)
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

int World::GetClosestLocationNotTarget(LOCATION type, const Vec2& pos, int not_target)
{
	int best = -1, index = 0;
	float dist, best_dist;

	for(vector<Location*>::iterator it = locations.begin(), end = locations.end(); it != end; ++it, ++index)
	{
		Location* loc = *it;
		if(!loc || loc->active_quest || loc->type != type)
			continue;
		if(((InsideLocation*)loc)->target == not_target)
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
			if(*it && Vec2::Distance(pt, (*it)->pos) < 24)
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

// Searches for location with selected spawn group
// If cleared respawns enemies
// If nothing found creates camp
int World::GetRandomSpawnLocation(const Vec2& pos, SPAWN_GROUP group, float range)
{
	int best_ok = -1, best_empty = -1, index = settlements;
	float ok_range, empty_range, dist;

	for(vector<Location*>::iterator it = locations.begin() + settlements, end = locations.end(); it != end; ++it, ++index)
	{
		if(!*it)
			continue;
		if(!(*it)->active_quest && ((*it)->type == L_DUNGEON || (*it)->type == L_CRYPT))
		{
			InsideLocation* inside = (InsideLocation*)*it;
			if((*it)->state == LS_CLEARED || inside->spawn == group || inside->spawn == SG_NONE)
			{
				dist = Vec2::Distance(pos, (*it)->pos);
				if(dist <= range)
				{
					if(inside->spawn == group)
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
		locations[best_empty]->spawn = group;
		locations[best_empty]->reset = true;
		return best_empty;
	}

	return CreateCamp(pos, group, range / 2);
}

int World::GetNearestLocation(const Vec2& pos, int flags, bool not_quest, int target_flags)
{
	assert(flags);

	float best_dist = 999.f;
	int best_index = -1, index = 0;

	for(vector<Location*>::iterator it = locations.begin(), end = locations.end(); it != end; ++it, ++index)
	{
		if(*it && IS_SET(flags, 1 << (*it)->type) && (!not_quest || !(*it)->active_quest))
		{
			if(target_flags != -1)
			{
				if((*it)->type == L_DUNGEON || (*it)->type == L_CRYPT)
				{
					if(!IS_SET(target_flags, 1 << ((InsideLocation*)(*it))->target))
						break;
				}
			}
			float dist = Vec2::Distance(pos, (*it)->pos);
			if(dist < best_dist)
			{
				best_dist = dist;
				best_index = index;
			}
		}
	}

	return best_index;
}

void World::ExitToMap()
{
	if(state == State::INSIDE_ENCOUNTER)
	{
		state = State::TRAVEL;
		current_location_index = -1;
		current_location = nullptr;
		L.location_index = -1;
		L.location = nullptr;
	}
	else
		state = State::ON_MAP;
}

void World::ChangeLevel(int index, bool encounter)
{
	state = encounter ? State::INSIDE_ENCOUNTER : State::INSIDE_LOCATION;
	current_location_index = index;
	current_location = locations[index];
	L.location_index = current_location_index;
	L.location = current_location;
}

void World::StartInLocation(Location* loc)
{
	locations.push_back(loc);
	current_location_index = locations.size() - 1;
	current_location = loc;
	state = State::INSIDE_LOCATION;
	loc->index = current_location_index;
	L.location_index = current_location_index;
	L.location = loc;
	L.is_open = true;
}

void World::StartEncounter()
{
	state = State::INSIDE_ENCOUNTER;
	current_location_index = encounter_loc;
	current_location = locations[current_location_index];
	L.location_index = current_location_index;
	L.location = current_location;
}

void World::Travel(int index)
{
	state = State::TRAVEL;
	current_location = nullptr;
	current_location_index = -1;
	L.location = nullptr;
	L.location_index = -1;
	travel_timer = 0.f;
	travel_day = 0;
	travel_start_pos = world_pos;
	travel_location_index = index;
	const Vec2& target_pos = locations[travel_location_index]->pos;
	travel_dir = Angle(world_pos.x, -world_pos.y, target_pos.x, -target_pos.y);
	reveal_timer = 0.f;

	if(Net::IsServer())
	{
		NetChange& c = Add1(Net::changes);
		c.type = NetChange::TRAVEL;
		c.id = index;
	}
}

void World::UpdateTravel(float dt)
{
	Game& game = Game::Get();

	travel_timer += dt;
	const Vec2& end_pt = locations[travel_location_index]->pos;
	float dist = Vec2::Distance(travel_start_pos, end_pt);
	if(travel_timer > travel_day)
	{
		// another day passed
		++travel_day;
		if(Net::IsLocal())
			Update(1, UM_TRAVEL);
	}

	if(travel_timer * 3 >= dist / TRAVEL_SPEED)
	{
		// end of travel
		if(game.IsLeader())
			EndTravel();
		else
			world_pos = end_pt;
	}
	else
	{
		Vec2 dir = end_pt - travel_start_pos;
		world_pos = travel_start_pos + dir * (travel_timer / dist * TRAVEL_SPEED * 3);

		// reveal nearby locations, check encounters
		reveal_timer += dt;
		if(Net::IsLocal() && reveal_timer >= 0.25f)
		{
			reveal_timer = 0;

			int what = -2;
			for(Location* ploc : locations)
			{
				if(!ploc)
					continue;
				Location& loc = *ploc;
				if(Vec2::Distance(world_pos, loc.pos) <= 32.f)
				{
					if(loc.state != LS_CLEARED)
					{
						int chance = 0;
						if(loc.type == L_FOREST)
						{
							chance = 1;
							what = -1;
						}
						else if(loc.spawn == SG_BANDITS)
						{
							chance = 3;
							what = loc.spawn;
						}
						else if(loc.spawn == SG_ORCS || loc.spawn == SG_GOBLINS)
						{
							chance = 2;
							what = loc.spawn;
						}
						encounter_chance += chance;
					}

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

			if(Rand() % 500 < ((int)encounter_chance) - 25 || DebugKey('E'))
			{
				encounter_chance = 0.f;
				StartEncounter(enc, what);
			}
		}
	}
}

void World::StartEncounter(int enc, int what)
{
	state = State::ENCOUNTER;
	locations[encounter_loc]->state = LS_UNKNOWN;
	if(Net::IsOnline())
		Net::PushChange(NetChange::UPDATE_MAP_POS);

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
		Quest_Crazies::State c_state = QM.quest_crazies->crazies_state;

		bool golem = (QM.quest_mages2->mages_state >= Quest_Mages2::State::Encounter
			&& QM.quest_mages2->mages_state < Quest_Mages2::State::Completed && Rand() % 3 == 0) || DebugKey('G');
		bool crazy = (c_state == Quest_Crazies::State::TalkedWithCrazy && (Rand() % 2 == 0 || DebugKey('S')));
		bool unk = (c_state >= Quest_Crazies::State::PickedStone && c_state < Quest_Crazies::State::End && (Rand() % 3 == 0 || DebugKey('S')));
		if(QM.quest_mages2->mages_state == Quest_Mages2::State::Encounter && Rand() % 2 == 0)
			golem = true;
		if(c_state == Quest_Crazies::State::PickedStone && Rand() % 2 == 0)
			unk = true;

		if(what == -2)
		{
			if(Rand() % 6 == 0)
				what = SG_BANDITS;
			else
				what = -3;
		}
		else if(Rand() % 3 == 0)
			what = -3;

		if(crazy || unk || golem || what == -3 || DebugKey(VK_SHIFT))
		{
			// special encounter
			encounter.mode = ENCOUNTER_SPECIAL;
			encounter.special = (SpecialEncounter)(Rand() % 6);
			if(unk)
				encounter.special = SE_UNK;
			else if(crazy)
				encounter.special = SE_CRAZY;
			else if(golem)
			{
				encounter.special = SE_GOLEM;
				QM.quest_mages2->paid = false;
			}
			else if(DEBUG_BOOL && Key.Focus())
			{
				if(Key.Down('I'))
					encounter.special = SE_CRAZY_HEROES;
				else if(Key.Down('B'))
					encounter.special = SE_BANDITS_VS_TRAVELERS;
				else if(Key.Down('C'))
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
			}
		}
		else
		{
			// combat encounter
			encounter.mode = ENCOUNTER_COMBAT;
			encounter.enemy = (SPAWN_GROUP)what;

			switch(what)
			{
			default:
				assert(0);
			case SG_BANDITS:
				text = txEncBandits;
				break;
			case -1:
				text = txEncAnimals;
				break;
			case SG_ORCS:
				text = txEncOrcs;
				break;
			case SG_GOBLINS:
				text = txEncGoblins;
				break;
			}
		}
	}

	gui->ShowEncounterMessage(text);
}

void World::EndTravel()
{
	if(state != State::TRAVEL)
		return;
	state = State::ON_MAP;
	current_location_index = travel_location_index;
	current_location = locations[travel_location_index];
	travel_location_index = -1;
	L.location_index = current_location_index;
	L.location = current_location;
	Location& loc = *L.location;
	loc.SetVisited();
	world_pos = loc.pos;
	if(Net::IsServer())
		Net::PushChange(NetChange::END_TRAVEL);
}

void World::Warp(int index)
{
	current_location_index = index;
	current_location = locations[current_location_index];
	L.location_index = current_location_index;
	L.location = current_location;
	Location& loc = *current_location;
	loc.SetVisited();
	world_pos = loc.pos;
}

Encounter* World::AddEncounter(int& index)
{
	for(int i = 0, size = (int)encounters.size(); i < size; ++i)
	{
		if(!encounters[i])
		{
			index = i;
			encounters[i] = new Encounter;
			return encounters[i];
		}
	}

	Encounter* enc = new Encounter;
	index = encounters.size();
	encounters.push_back(enc);
	return enc;
}

void World::RemoveEncounter(int index)
{
	assert(InRange(index, 0, (int)encounters.size() - 1) && encounters[index]);
	delete encounters[index];
	encounters[index] = nullptr;
}

Encounter* World::GetEncounter(int index)
{
	assert(InRange(index, 0, (int)encounters.size() - 1) && encounters[index]);
	return encounters[index];
}

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

	if(travel_dir < PI / 4 || travel_dir > 7.f / 4 * PI)
	{
		// east
		dir = PI / 2;
		if(travel_dir < PI / 4)
			pos = Vec3(map_size - dist, 0, Lerp(map_size / 2, map_size - dist, travel_dir / (PI / 4)));
		else
			pos = Vec3(map_size - dist, 0, Lerp(dist, map_size / 2, (travel_dir - (7.f / 4 * PI)) / (PI / 4)));
	}
	else if(travel_dir < 3.f / 4 * PI)
	{
		// north
		dir = 0;
		pos = Vec3(Lerp(dist, map_size - dist, 1.f - ((travel_dir - (1.f / 4 * PI)) / (PI / 2))), 0, map_size - dist);
	}
	else if(travel_dir < 5.f / 4 * PI)
	{
		// west
		dir = 3.f / 2 * PI;
		pos = Vec3(dist, 0, Lerp(dist, map_size - dist, 1.f - ((travel_dir - (3.f / 4 * PI)) / (PI / 2))));
	}
	else
	{
		// south
		dir = PI;
		pos = Vec3(Lerp(dist, map_size - dist, (travel_dir - (5.f / 4 * PI)) / (PI / 2)), 0, dist);
	}
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
}

// Mark all locations as known
void World::Reveal()
{
	for(Location* loc : locations)
	{
		if(loc)
			loc->SetKnown();
	}
}

void World::AddNews(cstring text)
{
	assert(text);

	News* n = new News;
	n->text = text;
	n->add_time = worldtime;

	news.push_back(n);
}

void World::AddBossLevel(const Int2& pos)
{
	if(Net::IsLocal())
		boss_levels.push_back(pos);
	else
		boss_level_mp = true;
}

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

bool World::CheckFirstCity()
{
	if(first_city)
	{
		first_city = false;
		return true;
	}
	return false;
}

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
