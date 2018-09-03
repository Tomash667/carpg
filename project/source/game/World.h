#pragma once

//-----------------------------------------------------------------------------
#include "Location.h"

//-----------------------------------------------------------------------------
// World handling
// General notes:
// + don't create settlements after GenerateWorld (currently it is hardcoded that they are before other locations - here and in Quest_SpreadNews)
// + currently only camps can be removed
class World
{
	friend class WorldMapGui;
public:
	enum class State
	{
		ON_MAP, // on map (current_location set)
		INSIDE_LOCATION, // inside location (current_location set)
		INSIDE_ENCOUNTER, // inside encounter location (current_location set)
		TRAVEL, // traveling on map (current_location is nullptr)
		ENCOUNTER // shown encounter message, waiting to close & load level (current_location is nullptr)
	};

	static const float TRAVEL_SPEED;

	// general
	void Init();
	void Cleanup();
	void OnNewGame();
	void Reset();
	void Update(int days);
	void DoWorldProgress(int days);
	void ExitToMap();
	void ChangeLevel(int index, bool encounter);
	void StartInLocation(Location* loc);
	void Warp(int index);
	void Reveal();

	// save/load
	void Save(GameWriter& f);
	void Load(GameReader& f, LoadingHandler& loading);
	void LoadOld(GameReader& f, LoadingHandler& loading, int part);
	void Write(BitStreamWriter& f);
	void WriteTime(BitStreamWriter& f);
	bool Read(BitStreamReader& f);
	void ReadTime(BitStreamReader& f);

	// world generation
	void GenerateWorld(int start_location_type = -1, int start_location_target = -1);
	int CreateCamp(const Vec2& pos, SPAWN_GROUP group, float range = 64.f, bool allow_exact = true);
	Location* CreateLocation(LOCATION type, int levels = -1, bool is_village = false);
	Location* CreateLocation(LOCATION type, const Vec2& pos, float range = 64.f, int target = -1, SPAWN_GROUP spawn = SG_RANDOM, bool allow_exact = true,
		int dungeon_levels = -1);
	typedef std::pair<LOCATION, bool>(*AddLocationsCallback)(uint index);
	void AddLocations(uint count, AddLocationsCallback clbk, float valid_dist, bool unique_name);
	int AddLocation(Location* loc);
	void AddLocationAtIndex(Location* loc);
	void RemoveLocation(int index);
	bool VerifyLocation(int index) const { return index >= 0 && index < (int)locations.size() && locations[index]; }
	void DeleteCamp(Camp* camp, bool remove = true);

	// datetime
	bool IsSameWeek(int worldtime2) const;
	cstring GetDate() const;
	int GetDay() const { return day; }
	int GetMonth() const { return month; }
	int GetYear() const { return year; }
	int GetWorldtime() const { return worldtime; }

	// world state
	State GetState() const { return state; }
	void SetState(State state) { this->state = state; }
	Location* GetCurrentLocation() const { return current_location; }
	int GetCurrentLocationIndex() const { return current_location_index; }
	const vector<Location*>& GetLocations() const { return locations; }
	Location* GetLocation(int index) const { assert(index >= 0 && index < (int)locations.size()); return locations[index]; }
	const Vec2& GetWorldPos() const { return world_pos; }
	void SetWorldPos(const Vec2& world_pos) { this->world_pos = world_pos; }
	int GetRandomSettlementIndex(int excluded = -1) const;
	int GetRandomSettlementIndex(const vector<int>& used, int type = 0) const;
	Location* GetRandomSettlement(int excluded = -1) const { return locations[GetRandomSettlementIndex(excluded)]; }
	int GetRandomFreeSettlementIndex(int excluded = -1) const;
	int GetRandomCityIndex(int excluded = -1) const;
	int GetClosestLocation(LOCATION type, const Vec2& pos, int target = -1);
	int GetClosestLocationNotTarget(LOCATION type, const Vec2& pos, int not_target);
	bool FindPlaceForLocation(Vec2& pos, float range = 64.f, bool allow_exact = true);
	int GetRandomSpawnLocation(const Vec2& pos, SPAWN_GROUP group, float range = 160.f);
	int GetNearestLocation(const Vec2& pos, int flags, bool not_quest, int target_flags = -1);
	int GetNearestSettlement(const Vec2& pos) { return GetNearestLocation(pos, (1 << L_CITY), false); }

	// travel
	void Travel(int index);
	void EndTravel();
	int GetTravelLocationIndex() const { return travel_location_index; }
	float GetTravelDir() const { return travel_dir; }
	void SetTravelDir(const Vec3& pos);
	void GetOutsideSpawnPoint(Vec3& pos, float& dir) const;

	// encounters
	void StartEncounter();
	Encounter* AddEncounter(int& index);
	void RemoveEncounter(int index);
	Encounter* GetEncounter(int index);
	Encounter* RecreateEncounter(int index);
	const vector<Encounter*>& GetEncounters() const { return encounters; }
	int GetEncounterLocationIndex() const { return encounter_loc; }
	float GetEncounterChance() const { return encounter_chance; }

	// news
	void AddNews(cstring text);
	const vector<News*>& GetNews() const { return news; }

	// boss levels
	void AddBossLevel(const Int2& pos = Int2::Zero);
	bool RemoveBossLevel(const Int2& pos = Int2::Zero);
	bool IsBossLevel(const Int2& pos = Int2::Zero) const;

	// misc
	bool CheckFirstCity();

private:
	State state;
	Location* current_location; // current location or nullptr
	int current_location_index; // current location index or -1
	int travel_location_index; // travel target where state is TRAVEL, ENCOUNTER or INSIDE_ENCOUNTER (-1 otherwise)
	vector<Location*> locations; // can be nullptr
	vector<Encounter*> encounters;
	vector<Int2> boss_levels; // levels with boss music (x-location index, y-dungeon level)
	uint settlements, // count and index below this value is city/village
		empty_locations, // counter
		encounter_loc, // encounter location index
		create_camp, // counter to create new random camps
		travel_day;
	Vec2 world_pos,
		travel_start_pos;
	float travel_timer,
		encounter_timer, // increase chance for encounter every 0.25 sec
		encounter_chance,
		travel_dir; // from which direction team will enter level after travel
	int year, // in game year, starts at 100
		month, // in game month, 0 to 11
		day, // in game day, 0 to 29
		worldtime; // number of passed game days, starts at 0
	vector<News*> news;
	cstring txDate, txRandomEncounter;
	bool first_city, // spawn more low level heroes in first city
		boss_level_mp; // used by clients instead boss_levels

	void LoadLocations(GameReader& f, LoadingHandler& loading);
	void LoadNews(GameReader& f);
};
extern World W;
