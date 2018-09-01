#include "Pch.h"
#include "GameCore.h"
#include "World.h"
#include "BitStreamFunc.h"
#include "Language.h"
#include "LocationHelper.h"

#include "Camp.h"
#include "CaveLocation.h"
#include "City.h"
#include "MultiInsideLocation.h"


//-----------------------------------------------------------------------------
const int start_year = 100;
const float world_size = 600.f;
const float world_border = 16.f;
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
void World::Init()
{
	txDate = Str("dateFormat");
	txRandomEncounter = Str("randomEncounter");
}

void World::OnNewGame()
{
	empty_locations = 0;
	year = start_year;
	day = 0;
	month = 0;
	worldtime = 0;
}

void World::Update(int days)
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

			locations.push_back(l);
			break;
		}
	}
}

uint World::GenerateWorld(int start_location_type, int start_location_target)
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
		loc->pos = Vec2(-1000, -1000);
		loc->name = txRandomEncounter;
		loc->state = LS_VISITED;
		loc->image = LI_FOREST;
		loc->type = L_ENCOUNTER;
		encounter_loc = locations.size();
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
				inside->spawn = SG_NIEUMARLI;
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
	return start_location;
}

void World::Save(FileWriter& f)
{
	f << state;
	f << year;
	f << month;
	f << day;
	f << worldtime;
}

void World::Load(FileReader& f)
{
	f >> state;
	f >> year;
	f >> month;
	f >> day;
	f >> worldtime;
}

void World::LoadOld(FileReader& f, int part)
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
	}
}

void World::WriteTime(BitStreamWriter& f)
{
	f << worldtime;
	f.WriteCasted<byte>(day);
	f.WriteCasted<byte>(month);
	f << year;
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

void World::ExitToMap()
{
	/*assert(state == State::INSIDE_LOCATION);
	if(current_location)
	if(world_state == WS_ENCOUNTER)
		world_state = WS_TRAVEL;
	else if(world_state != WS_TRAVEL)
		world_state = WS_MAIN;*/
}
